/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/speex_resampler.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogReportInterval = 20 * core::Second;

inline const char* get_error_msg(int err) {
    if (err == 5) {
        // this code is missing from speex_resampler_strerror()
        return "Ratio overflow.";
    }
    return speex_resampler_strerror(err);
}

inline int get_quality(ResamplerProfile profile) {
    switch (profile) {
    case ResamplerProfile_Low:
        return 1;

    case ResamplerProfile_Medium:
        return 5;

    case ResamplerProfile_High:
        return 10;
    }

    roc_panic("speex resampler: unexpected profile");
}

} // namespace

SpeexResampler::SpeexResampler(const ResamplerConfig& config,
                               const SampleSpec& in_spec,
                               const SampleSpec& out_spec,
                               FrameFactory& frame_factory,
                               core::IArena& arena)
    : IResampler(arena)
    , speex_state_(NULL)
    , num_ch_((spx_uint32_t)in_spec.num_channels())
    , in_frame_size_(0)
    , in_frame_pos_(0)
    , initial_out_countdown_(0)
    , initial_in_latency_(0)
    , in_latency_diff_(0)
    , report_limiter_(LogReportInterval)
    , init_status_(status::NoStatus) {
    if (!in_spec.is_complete() || !out_spec.is_complete() || !in_spec.is_raw()
        || !out_spec.is_raw()) {
        roc_panic("speex resampler: required complete sample specs with raw format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec).c_str(),
                  sample_spec_to_str(out_spec).c_str());
    }

    if (in_spec.channel_set() != out_spec.channel_set()) {
        roc_panic("speex resampler: required identical input and output channel sets:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec).c_str(),
                  sample_spec_to_str(out_spec).c_str());
    }

    const int quality = get_quality(config.profile);

    int err = 0;
    speex_state_ =
        speex_resampler_init(num_ch_, (spx_uint32_t)in_spec.sample_rate(),
                             (spx_uint32_t)in_spec.sample_rate(), quality, &err);
    if (err != RESAMPLER_ERR_SUCCESS || !speex_state_) {
        roc_log(LogError, "speex resampler: speex_resampler_init(): [%d] %s", err,
                get_error_msg(err));
        init_status_ = err == RESAMPLER_ERR_ALLOC_FAILED ? status::StatusNoMem
                                                         : status::StatusBadConfig;
        return;
    }

    initial_out_countdown_ = (size_t)speex_resampler_get_output_latency(speex_state_);
    initial_in_latency_ = (size_t)speex_resampler_get_input_latency(speex_state_);

    in_frame_size_ = in_frame_pos_ = std::min(
        initial_in_latency_ * in_spec.num_channels(), frame_factory.raw_buffer_size());

    roc_log(LogDebug,
            "speex resampler: initializing:"
            " profile=%s quality=%d frame_size=%lu channels_num=%lu",
            resampler_profile_to_str(config.profile), quality,
            (unsigned long)in_frame_size_, (unsigned long)num_ch_);

    if (!(in_frame_ = frame_factory.new_raw_buffer())) {
        roc_log(LogError, "speex resampler: can't allocate frame buffer");
        init_status_ = status::StatusNoMem;
        return;
    }
    in_frame_.reslice(0, in_frame_size_);

    init_status_ = status::StatusOK;
}

SpeexResampler::~SpeexResampler() {
    if (speex_state_) {
        speex_resampler_destroy(speex_state_);
    }
}

status::StatusCode SpeexResampler::init_status() const {
    return init_status_;
}

bool SpeexResampler::set_scaling(size_t input_rate, size_t output_rate, float mult) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (input_rate == 0 || output_rate == 0 || mult <= 0
        || input_rate * mult > (float)ROC_MAX_OF(spx_uint32_t)
        || output_rate * mult > (float)ROC_MAX_OF(spx_uint32_t)) {
        roc_log(LogError,
                "speex resampler: scaling out of range: in_rate=%lu out_rate=%lu mult=%e",
                (unsigned long)input_rate, (unsigned long)output_rate, (double)mult);
        return false;
    }

    // We need to provide speex with integer numerator and denumerator, where numerator
    // is proportional to `input_rate * mult` and denumerator is proportional to
    // `output_rate`.
    //
    // If we just multiply rate by `mult` and round result to integer, the precision
    // will be quite low, because `mult` is very close to 1.0 (because it's used to
    // compensate clock drift which is slow).
    //
    // To increase precision, we first multiply input and output rates by same `base`.
    // The higher is the base, the better is scaling precision. E.g. if `base` is
    // 1'000'000, we could represent 6 digits of fractional part of `mult` without
    // rounding errors.
    //
    // Unfortunately, speex does not allow numerator and denumerator to be larger
    // than certain value. If it happens, either speex_resampler_set_rate_frac()
    // returns error, or it succeeds, but overflows happen during resampling.
    //
    // To work around this, we use floating-point `base` and compute maximum "safe"
    // value which will not cause overflows in speex.
    //
    // We also keep number of digits in fractional part of `base` small, to be sure
    // that multiplying rates by `base` won't introduce its own rounding errors.
    //
    // Another important feature of these formulas is that when `mult` is exactly 1.0,
    // `numerator / denumerator` will be exactly equal to `input_rate / output_rate`.
    // For example, when sender uses resampler without clock drift compensation, it
    // sets `mult` to 1.0 and needs to be sure that resampler will convert between
    // rates exactly as requested, without rounding errors.

    const float max_numerator = 60000; // selected empirically
    const float base_frac = 10;        // no more than 1 digit in fractional part

    const float base = (input_rate < max_numerator && output_rate < max_numerator)
        ? roundf(max_numerator / std::max(input_rate, output_rate) * base_frac)
            / base_frac
        : 1.0f;

    const spx_uint32_t ratio_num = spx_uint32_t(roundf(float(input_rate) * mult * base));
    const spx_uint32_t ratio_den = spx_uint32_t(roundf(float(output_rate) * base));

    if (ratio_num == 0 || ratio_den == 0) {
        roc_log(LogError, "speex resampler: invalid scaling");
        return false;
    }

    const int err = speex_resampler_set_rate_frac(speex_state_, ratio_num, ratio_den,
                                                  spx_uint32_t(roundf(input_rate * mult)),
                                                  spx_uint32_t(output_rate));

    if (err != RESAMPLER_ERR_SUCCESS) {
        roc_log(LogError,
                "speex resampler: speex_resampler_set_rate_frac(%d/%d, %d/%d): [%d] %s",
                (int)ratio_num, (int)ratio_den, int(roundf(input_rate * mult)),
                int(output_rate), err, get_error_msg(err));
        return false;
    }

    in_latency_diff_ = (ssize_t)speex_resampler_get_input_latency(speex_state_)
        - (ssize_t)initial_in_latency_;

    return true;
}

const core::Slice<sample_t>& SpeexResampler::begin_push_input() {
    roc_panic_if(init_status_ != status::StatusOK);
    roc_panic_if(in_frame_pos_ != in_frame_size_);

    return in_frame_;
}

void SpeexResampler::end_push_input() {
    roc_panic_if(init_status_ != status::StatusOK);

    in_frame_pos_ = 0;
}

size_t SpeexResampler::pop_output(sample_t* out_buf, size_t out_bufsz) {
    roc_panic_if(init_status_ != status::StatusOK);

    const sample_t* in_frame_data = in_frame_.data();

    sample_t* out_frame_data = out_buf;
    const spx_uint32_t out_frame_size = (spx_uint32_t)out_bufsz;
    spx_uint32_t out_frame_pos = 0;

    while (in_frame_pos_ != in_frame_size_ && out_frame_pos != out_frame_size) {
        spx_uint32_t remaining_out = (out_frame_size - out_frame_pos) / num_ch_;
        spx_uint32_t remaining_in = (in_frame_size_ - in_frame_pos_) / num_ch_;

        const int err = speex_resampler_process_interleaved_float(
            speex_state_, in_frame_data + in_frame_pos_, &remaining_in,
            out_frame_data + out_frame_pos, &remaining_out);

        if (err != RESAMPLER_ERR_SUCCESS) {
            roc_panic(
                "speex resampler: speex_resampler_process_interleaved_float(): [%d] %s",
                err, get_error_msg(err));
        }

        in_frame_pos_ += remaining_in * num_ch_;

        // Speex inserts zero samples in the beginning of the stream, corresponding to
        // its latency. Other resampler backends don't do it, instead, in the beginning
        // they request more samples (by returning zero from pop) until they accumulate
        // required latency.
        //
        // Here we adjust speex behavior to be in-line with other backends. It allows
        // us to perform latency and timestamp calculations uniformly for all backends.
        if (initial_out_countdown_) {
            const size_t n_samples =
                std::min((size_t)remaining_out, initial_out_countdown_);

            remaining_out -= n_samples;
            initial_out_countdown_ -= n_samples;
        }

        out_frame_pos += remaining_out * num_ch_;

        roc_panic_if(in_frame_pos_ > in_frame_size_);
        roc_panic_if(out_frame_pos > out_frame_size);
    }

    report_stats_();

    return (size_t)out_frame_pos;
}

float SpeexResampler::n_left_to_process() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return float(in_frame_size_ - in_frame_pos_) + float(in_latency_diff_);
}

void SpeexResampler::report_stats_() {
    if (!speex_state_) {
        return;
    }

    if (!report_limiter_.allow()) {
        return;
    }

    spx_uint32_t ratio_num = 0;
    spx_uint32_t ratio_den = 0;
    speex_resampler_get_ratio(speex_state_, &ratio_num, &ratio_den);

    spx_uint32_t in_rate = 0;
    spx_uint32_t out_rate = 0;
    speex_resampler_get_rate(speex_state_, &in_rate, &out_rate);

    const int in_latency = speex_resampler_get_input_latency(speex_state_);

    roc_log(LogDebug,
            "speex resampler:"
            " ratio=%u/%u rates=%u/%u latency=%d latency_diff=%d",
            (unsigned int)ratio_num, (unsigned int)ratio_den, (unsigned int)in_rate,
            (unsigned int)out_rate, (int)in_latency, (int)in_latency_diff_);
}

} // namespace audio
} // namespace roc
