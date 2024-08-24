/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/decimation_resampler.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

namespace {

const core::nanoseconds_t LogReportInterval = 20 * core::Second;

const size_t InputFrameSize = 16;

} // namespace

DecimationResampler::DecimationResampler(
    const core::SharedPtr<IResampler>& inner_resampler,
    const SampleSpec& in_spec,
    const SampleSpec& out_spec,
    FrameFactory& frame_factory,
    core::IArena& arena)
    : IResampler(arena)
    , inner_resampler_(inner_resampler)
    , use_inner_resampler_(in_spec.sample_rate() != out_spec.sample_rate())
    , input_spec_(in_spec)
    , output_spec_(out_spec)
    , multiplier_(1.0f)
    , num_ch_(in_spec.num_channels())
    , in_size_(0)
    , in_pos_(0)
    , out_acc_(0)
    , total_count_(0)
    , decim_count_(0)
    , report_limiter_(LogReportInterval)
    , init_status_(status::NoStatus) {
    if (!in_spec.is_complete() || !out_spec.is_complete() || !in_spec.is_raw()
        || !out_spec.is_raw()) {
        roc_panic("decimation resampler: required complete sample specs with raw format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec).c_str(),
                  sample_spec_to_str(out_spec).c_str());
    }

    if (in_spec.channel_set() != out_spec.channel_set()) {
        roc_panic(
            "decimation resampler: required identical input and output channel sets:"
            " in_spec=%s out_spec=%s",
            sample_spec_to_str(in_spec).c_str(), sample_spec_to_str(out_spec).c_str());
    }

    roc_log(LogDebug,
            "decimation resampler: initializing: "
            " frame_size=%lu num_ch=%lu use_inner_resampler=%d",
            (unsigned long)InputFrameSize, (unsigned long)num_ch_,
            (int)use_inner_resampler_);

    if (frame_factory.raw_buffer_size() < InputFrameSize * num_ch_) {
        roc_log(LogError, "decimation resampler: can't allocate temporary buffer");
        init_status_ = status::StatusNoMem;
        return;
    }

    in_buf_ = frame_factory.new_raw_buffer();
    if (!in_buf_) {
        roc_log(LogError, "decimation resampler: can't allocate temporary buffer");
        init_status_ = status::StatusNoMem;
        return;
    }
    in_buf_.reslice(0, InputFrameSize * num_ch_);

    last_buf_ = frame_factory.new_raw_buffer();
    if (!last_buf_) {
        roc_log(LogError, "decimation resampler: can't allocate temporary buffer");
        init_status_ = status::StatusNoMem;
        return;
    }
    last_buf_.reslice(0, num_ch_);

    memset(last_buf_.data(), 0, last_buf_.size() * sizeof(sample_t));

    init_status_ = status::StatusOK;
}

DecimationResampler::~DecimationResampler() {
}

status::StatusCode DecimationResampler::init_status() const {
    return init_status_;
}

bool DecimationResampler::set_scaling(size_t input_rate,
                                      size_t output_rate,
                                      float multiplier) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (input_rate == 0 || output_rate == 0
        || multiplier <= 0
        // no more than num_ch insertions/removals per input frame,
        // because we insert or remove only one sample per time
        || std::abs(in_buf_.size() / multiplier - in_buf_.size()) > num_ch_) {
        roc_log(LogError,
                "decimation resampler:"
                " scaling out of range: in_rate=%lu out_rate=%lu mult=%e",
                (unsigned long)input_rate, (unsigned long)output_rate,
                (double)multiplier);
        return false;
    }

    use_inner_resampler_ = (input_rate != output_rate);

    if (use_inner_resampler_) {
        // always pass 1.0 instead of multiplier to inner resampler
        if (!inner_resampler_->set_scaling(input_rate, output_rate, 1.0f)) {
            return false;
        }
    }

    input_spec_.set_sample_rate(input_rate);
    output_spec_.set_sample_rate(output_rate);

    multiplier_ = multiplier;

    return true;
}

const core::Slice<sample_t>& DecimationResampler::begin_push_input() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (use_inner_resampler_) {
        // return buffer of inner resampler
        return inner_resampler_->begin_push_input();
    }

    // return our buffer
    return in_buf_;
}

void DecimationResampler::end_push_input() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (use_inner_resampler_) {
        // start reading from inner resampler
        inner_resampler_->end_push_input();
        return;
    }

    // start reading from our buffer
    in_size_ = in_buf_.size();
    in_pos_ = 0;
    out_acc_ += in_size_ / multiplier_;
}

size_t DecimationResampler::pop_output(sample_t* out_data, size_t out_size) {
    roc_panic_if(init_status_ != status::StatusOK);

    size_t out_pos = 0;

    while (out_pos < out_size) {
        // self-check
        roc_panic_if_not(in_size_ % num_ch_ == 0 && in_pos_ % num_ch_ == 0
                         && in_pos_ <= in_size_);
        roc_panic_if_not(out_size % num_ch_ == 0 && out_pos % num_ch_ == 0
                         && out_pos <= out_size);

        if (in_pos_ == in_size_ && use_inner_resampler_) {
            // no more samples in input frame, but maybe inner resampler has more?
            // try to refill our buffer and start reading from it
            in_size_ = inner_resampler_->pop_output(in_buf_.data(), in_buf_.size());
            in_pos_ = 0;
            out_acc_ += in_size_ / multiplier_;
        }

        if (in_pos_ == in_size_) {
            // no more samples in input frame and inner resampler
            // caller should push more input samples
            break;
        }

        if (floorf(out_acc_) >= float(in_size_ - in_pos_) + num_ch_) {
            // accumulator is ahead of input by at least num_ch samples
            // duplicate num_ch input samples to compensate
            memcpy(out_data + out_pos, last_buf_.data(), num_ch_ * sizeof(sample_t));
            out_pos += num_ch_;
            out_acc_ -= num_ch_;
            // for reports
            decim_count_ += num_ch_;
        } else if (ceilf(out_acc_) <= float(in_size_ - in_pos_) - num_ch_) {
            // accumulator is behind of input by at least num_ch samples
            // skip num_ch input samples to compensate
            in_pos_ += num_ch_;
            // for reports
            decim_count_ += num_ch_;
        }

        // copy input samples to output
        const size_t copy_size = std::min(in_size_ - in_pos_, out_size - out_pos);

        if (copy_size != 0) {
            roc_panic_if_not(copy_size % num_ch_ == 0);

            memcpy(out_data + out_pos, in_buf_.data() + in_pos_,
                   copy_size * sizeof(sample_t));

            out_pos += copy_size;
            in_pos_ += copy_size;
            out_acc_ -= copy_size;

            // remember last num_ch samples of last frame
            memcpy(last_buf_.data(), out_data + out_pos - num_ch_,
                   num_ch_ * sizeof(sample_t));
        }
    }

    // for reports
    total_count_ += out_pos;

    report_stats_();

    return out_pos;
}

float DecimationResampler::n_left_to_process() const {
    roc_panic_if(init_status_ != status::StatusOK);

    // how much samples are pending in our buffer
    float n_samples = float(in_size_ - in_pos_) / output_spec_.sample_rate()
        * input_spec_.sample_rate();

    if (use_inner_resampler_) {
        // how much samples are pending in inner resampler
        n_samples += inner_resampler_->n_left_to_process();
    }

    return n_samples;
}

void DecimationResampler::report_stats_() {
    if (!report_limiter_.allow()) {
        return;
    }

    // number of insertions/duplications per channel per second
    const float decim_ratio = ((float)decim_count_ / num_ch_)
        / ((float)output_spec_.samples_overall_2_ns(total_count_) / core::Second);

    total_count_ = 0;
    decim_count_ = 0;

    roc_log(LogDebug, "decimation resampler: mult=%.6f ratio=%.3f samples/sec",
            (double)multiplier_, (double)decim_ratio);
}

} // namespace audio
} // namespace roc
