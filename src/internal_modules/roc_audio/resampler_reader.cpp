/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_reader.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ResamplerReader::ResamplerReader(IFrameReader& frame_reader,
                                 FrameFactory& frame_factory,
                                 IResampler& resampler,
                                 const SampleSpec& in_spec,
                                 const SampleSpec& out_spec)
    : frame_factory_(frame_factory)
    , frame_reader_(frame_reader)
    , resampler_(resampler)
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , in_buf_pos_(0)
    , last_in_cts_(0)
    , scaling_(1.0f)
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_complete() || !out_spec_.is_complete() || !in_spec_.is_raw()
        || !out_spec_.is_raw()) {
        roc_panic("resampler reader: required complete sample specs with raw format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.channel_set() != out_spec_.channel_set()) {
        roc_panic("resampler reader: required identical input and output channel sets:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if ((init_status_ = resampler_.init_status()) != status::StatusOK) {
        return;
    }

    if (!resampler_.set_scaling(in_spec_.sample_rate(), out_spec_.sample_rate(), 1.0f)) {
        init_status_ = status::StatusBadConfig;
        return;
    }

    in_frame_ = frame_factory_.allocate_frame(0);
    if (!in_frame_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode ResamplerReader::init_status() const {
    return init_status_;
}

bool ResamplerReader::set_scaling(float multiplier) {
    roc_panic_if(init_status_ != status::StatusOK);

    scaling_ = multiplier;

    return resampler_.set_scaling(in_spec_.sample_rate(), out_spec_.sample_rate(),
                                  multiplier);
}

status::StatusCode ResamplerReader::read(Frame& out_frame,
                                         packet::stream_timestamp_t requested_duration,
                                         FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    const packet::stream_timestamp_t capped_duration = out_spec_.cap_frame_duration(
        requested_duration, frame_factory_.byte_buffer_size());

    if (!frame_factory_.reallocate_frame(
            out_frame, out_spec_.stream_timestamp_2_bytes(capped_duration))) {
        return status::StatusNoMem;
    }

    out_frame.set_raw(true);

    size_t out_pos = 0;

    while (out_pos < out_frame.num_raw_samples()) {
        const size_t out_remain = out_frame.num_raw_samples() - out_pos;

        const size_t num_popped =
            resampler_.pop_output(out_frame.raw_samples() + out_pos, out_remain);

        if (num_popped < out_remain) {
            const status::StatusCode code = push_input_(mode);
            if (code != status::StatusOK) {
                return code;
            }
        }

        out_pos += num_popped;
    }

    out_frame.set_duration(capped_duration);
    out_frame.set_capture_timestamp(capture_ts_(out_frame));

    return capped_duration == requested_duration ? status::StatusOK : status::StatusPart;
}

status::StatusCode ResamplerReader::push_input_(FrameReadMode mode) {
    // Resampler returns buffer where we should write input samples.
    if (!in_buf_) {
        in_buf_ = resampler_.begin_push_input();
        in_buf_pos_ = 0;
    }

    while (in_buf_pos_ < in_buf_.size()) {
        const packet::stream_timestamp_t duration = packet::stream_timestamp_t(
            (in_buf_.size() - in_buf_pos_) / in_spec_.num_channels());

        if (!frame_factory_.reallocate_frame(
                *in_frame_, in_spec_.stream_timestamp_2_bytes(duration))) {
            return status::StatusNoMem;
        }

        // If we got StatusPart, we repeat reading until resampler input buffer is full.
        // If we got StatusDrain, we exit, but remember buffer state and can continue
        // next time when read() is called.
        const status::StatusCode code = frame_reader_.read(*in_frame_, duration, mode);
        if (code != status::StatusOK && code != status::StatusPart) {
            return code;
        }

        in_spec_.validate_frame(*in_frame_);

        memcpy(in_buf_.data() + in_buf_pos_, in_frame_->raw_samples(),
               in_frame_->num_raw_samples() * sizeof(sample_t));

        in_buf_pos_ += in_frame_->num_raw_samples();
    }

    // Tell resampler that input samples are ready.
    resampler_.end_push_input();

    in_buf_.reset();
    in_buf_pos_ = 0;

    const core::nanoseconds_t in_cts = in_frame_->capture_timestamp();
    if (in_cts > 0) {
        // Remember timestamp of last sample of last input frame.
        last_in_cts_ =
            in_cts + in_spec_.samples_overall_2_ns(in_frame_->num_raw_samples());
    }

    return status::StatusOK;
}

// Compute timestamp of first sample of current output frame.
// We have timestamps in input frames, and we should find to
// which time our output frame does correspond in input stream.
core::nanoseconds_t ResamplerReader::capture_ts_(const Frame& out_frame) {
    if (last_in_cts_ == 0) {
        // We didn't receive input frame with non-zero cts yet,
        // so for now we keep cts zero.
        return 0;
    }

    // Get timestamp of last sample of last input frame pushed to resampler.
    // Now we have tail of input stream.
    core::nanoseconds_t out_cts = last_in_cts_;

    // Subtract number of input samples that resampler haven't processed yet.
    // Now we have point in input stream corresponding to tail of output frame.
    out_cts -= in_spec_.fract_samples_overall_2_ns(resampler_.n_left_to_process());

    // Subtract length of current output frame multiplied by scaling.
    // Now we have point in input stream corresponding to head of output frame.
    out_cts -= core::nanoseconds_t(
        out_spec_.samples_overall_2_ns(out_frame.num_raw_samples()) * scaling_);

    if (out_cts < 0) {
        // Input frame cts was very close to zero (unix epoch), in this case we
        // avoid producing negative cts until it grows a bit.
        return 0;
    }

    return out_cts;
}

} // namespace audio
} // namespace roc
