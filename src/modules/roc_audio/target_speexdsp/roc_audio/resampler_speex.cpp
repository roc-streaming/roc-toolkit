/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_speex.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogReportInterval = 20 * core::Second;

inline const char* get_error_msg(int err) {
    if (err == 5) {
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

SpeexResampler::SpeexResampler(core::IAllocator&,
                               core::BufferPool<sample_t>& buffer_pool,
                               ResamplerProfile profile,
                               core::nanoseconds_t frame_length,
                               size_t sample_rate,
                               packet::channel_mask_t channels)
    : speex_state_(NULL)
    , in_frame_size_(
          (spx_uint32_t)packet::ns_to_size(frame_length, sample_rate, channels))
    , in_frame_pos_(in_frame_size_)
    , num_ch_((spx_uint32_t)packet::num_channels(channels))
    , rate_limiter_(LogReportInterval)
    , valid_(false) {
    if (num_ch_ == 0 || in_frame_size_ == 0) {
        return;
    }

    const int quality = get_quality(profile);

    roc_log(LogDebug,
            "speex resampler: initializing: "
            "quality=%d frame_size=%lu channels_num=%lu",
            quality, (unsigned long)in_frame_size_, (unsigned long)num_ch_);

    if (!(in_frame_ = new (buffer_pool) core::Buffer<sample_t>(buffer_pool))) {
        roc_log(LogError, "speex resampler: can't allocate frame buffer");
        return;
    }
    in_frame_.resize(in_frame_size_);

    int err = 0;
    speex_state_ = speex_resampler_init(num_ch_, (spx_uint32_t)sample_rate,
                                        (spx_uint32_t)sample_rate, quality, &err);
    if (err != RESAMPLER_ERR_SUCCESS || !speex_state_) {
        roc_log(LogError, "speex resampler: speex_resampler_init(): [%d] %s", err,
                get_error_msg(err));
        return;
    }

    valid_ = true;
}

SpeexResampler::~SpeexResampler() {
    if (speex_state_) {
        speex_resampler_destroy(speex_state_);
    }
}

bool SpeexResampler::valid() const {
    return valid_;
}

bool SpeexResampler::set_scaling(size_t input_rate, size_t output_rate, float mult) {
    // Maximum possible precision for reasanoble rate and scaling values.
    // Not ideal, but larger precision will cause overflow error in speex.
    enum { Precision = 50000 };

    if (input_rate == 0 || output_rate == 0) {
        roc_log(LogError, "speex resampler: invalid rate");
        return false;
    }

    if (mult <= 0 || mult > (0xffffffff / Precision)) {
        roc_log(LogError, "speex resampler: invalid scaling");
        return false;
    }

    const spx_uint32_t ratio_num = spx_uint32_t(mult * Precision);

    const spx_uint32_t ratio_den =
        spx_uint32_t(float(output_rate) / float(input_rate) * Precision);

    if (ratio_num == 0 || ratio_den == 0) {
        roc_log(LogError, "speex resampler: invalid scaling");
        return false;
    }

    const int err = speex_resampler_set_rate_frac(speex_state_, ratio_num, ratio_den,
                                                  spx_uint32_t(float(input_rate) * mult),
                                                  spx_uint32_t(output_rate));

    if (err != RESAMPLER_ERR_SUCCESS) {
        roc_log(LogError,
                "speex resampler: speex_resampler_set_rate_frac(%d,%d,%d,%d): [%d] %s",
                (int)ratio_num, (int)ratio_den, int(float(input_rate) * mult),
                int(output_rate), err, get_error_msg(err));
        return false;
    }

    return true;
}

const core::Slice<sample_t>& SpeexResampler::begin_push_input() {
    roc_panic_if_not(in_frame_pos_ == in_frame_size_);

    return in_frame_;
}

void SpeexResampler::end_push_input() {
    in_frame_pos_ = 0;
}

size_t SpeexResampler::pop_output(Frame& out) {
    const spx_uint32_t out_frame_size = spx_uint32_t(out.size());
    sample_t* out_frame_data = out.data();
    spx_uint32_t out_frame_pos = 0;

    sample_t* in_frame_data = in_frame_.data();

    roc_panic_if(!out_frame_data);
    roc_panic_if(!in_frame_data);

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
        out_frame_pos += remaining_out * num_ch_;

        roc_panic_if(in_frame_pos_ > in_frame_size_);
        roc_panic_if(out_frame_pos > out_frame_size);
    }

    report_stats_();

    return (size_t)out_frame_pos;
}

void SpeexResampler::report_stats_() {
    if (!speex_state_) {
        return;
    }

    if (!rate_limiter_.allow()) {
        return;
    }

    spx_uint32_t ratio_num = 0;
    spx_uint32_t ratio_den = 0;
    speex_resampler_get_ratio(speex_state_, &ratio_num, &ratio_den);

    spx_uint32_t in_rate = 0;
    spx_uint32_t out_rate = 0;
    speex_resampler_get_rate(speex_state_, &in_rate, &out_rate);

    const int in_latency = speex_resampler_get_input_latency(speex_state_);
    const int out_latency = speex_resampler_get_output_latency(speex_state_);

    roc_log(
        LogDebug,
        "speex resampler:"
        " ratio_num=%u ratio_den=%u in_rate=%u out_rate=%u in_latency=%d out_latency=%d",
        (unsigned int)ratio_num, (unsigned int)ratio_den, (unsigned int)in_rate,
        (unsigned int)out_rate, (int)in_latency, (int)out_latency);
}

} // namespace audio
} // namespace roc
