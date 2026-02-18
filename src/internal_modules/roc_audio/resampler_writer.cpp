/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_writer.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

ResamplerWriter::ResamplerWriter(IFrameWriter& frame_writer,
                                 FrameFactory& frame_factory,
                                 IResampler& resampler,
                                 const SampleSpec& in_sample_spec,
                                 const SampleSpec& out_sample_spec)
    : frame_factory_(frame_factory)
    , frame_writer_(frame_writer)
    , resampler_(resampler)
    , in_spec_(in_sample_spec)
    , out_spec_(out_sample_spec)
    , in_buf_pos_(0)
    , out_frame_pos_(0)
    , scaling_(1.f)
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_complete() || !out_spec_.is_complete() || !in_spec_.is_raw()
        || !out_spec_.is_raw()) {
        roc_panic("resampler writer: required complete sample specs with raw format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.channel_set() != out_spec_.channel_set()) {
        roc_panic("resampler writer: required identical input and output channel sets:"
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

    out_frame_ = frame_factory_.allocate_frame(0);
    if (!out_frame_) {
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

status::StatusCode ResamplerWriter::init_status() const {
    return init_status_;
}

bool ResamplerWriter::set_scaling(float multiplier) {
    roc_panic_if(init_status_ != status::StatusOK);

    scaling_ = multiplier;

    return resampler_.set_scaling(in_spec_.sample_rate(), out_spec_.sample_rate(),
                                  multiplier);
}

status::StatusCode ResamplerWriter::write(Frame& in_frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    in_spec_.validate_frame(in_frame);

    const size_t in_frame_size = in_frame.num_raw_samples();
    size_t in_frame_pos = 0;

    while (in_frame_pos < in_frame_size) {
        if (out_frame_pos_ == 0) {
            if (!frame_factory_.reallocate_frame(*out_frame_,
                                                 frame_factory_.byte_buffer_size())) {
                return status::StatusNoMem;
            }

            out_frame_->set_raw(true);
        }

        const size_t out_frame_remain = out_frame_->num_raw_samples() - out_frame_pos_;

        if (out_frame_remain != 0) {
            const size_t num_popped = resampler_.pop_output(
                out_frame_->raw_samples() + out_frame_pos_, out_frame_remain);

            if (num_popped < out_frame_remain) {
                in_frame_pos += push_input_(in_frame, in_frame_pos);
            }

            out_frame_pos_ += num_popped;
        }

        if (out_frame_pos_ == out_frame_->num_raw_samples()
            || (out_frame_pos_ != 0 && in_frame_pos == in_frame_size)) {
            const status::StatusCode code = write_output_(in_frame, in_frame_pos);
            if (code != status::StatusOK) {
                return code;
            }

            out_frame_pos_ = 0;
        }
    }

    return status::StatusOK;
}

status::StatusCode ResamplerWriter::write_output_(const Frame& in_frame,
                                                  size_t in_frame_pos) {
    const packet::stream_timestamp_t duration =
        packet::stream_timestamp_t(out_frame_pos_ / out_spec_.num_channels());

    out_frame_->set_flags(in_frame.flags());
    out_frame_->set_num_raw_samples(out_frame_pos_);
    out_frame_->set_duration(duration);
    out_frame_->set_capture_timestamp(capture_ts_(in_frame, in_frame_pos));

    return frame_writer_.write(*out_frame_);
}

size_t ResamplerWriter::push_input_(Frame& in_frame, size_t in_frame_pos) {
    if (!in_buf_) {
        in_buf_ = resampler_.begin_push_input();
        in_buf_pos_ = 0;
    }

    const size_t num_copy =
        std::min(in_frame.num_raw_samples() - in_frame_pos, in_buf_.size() - in_buf_pos_);

    memcpy(in_buf_.data() + in_buf_pos_, in_frame.raw_samples() + in_frame_pos,
           num_copy * sizeof(sample_t));

    in_buf_pos_ += num_copy;

    if (in_buf_pos_ == in_buf_.size()) {
        resampler_.end_push_input();

        in_buf_.reset();
        in_buf_pos_ = 0;
    }

    return num_copy;
}

// Compute timestamp of first sample of current output frame.
// We have timestamps in input frames, and we should find to
// which time our output frame does correspond in input stream.
core::nanoseconds_t ResamplerWriter::capture_ts_(const Frame& in_frame,
                                                 size_t in_frame_pos) {
    if (in_frame.capture_timestamp() == 0) {
        // We didn't receive input frame with non-zero cts yet,
        // so for now we keep cts zero.
        return 0;
    }

    // Get timestamp of first sample of last input frame pushed to resampler.
    core::nanoseconds_t out_cts = in_frame.capture_timestamp();

    // Add number of samples copied from input frame to our buffer and then to resampler.
    // Now we have tail of input stream.
    out_cts += in_spec_.samples_overall_2_ns(in_frame_pos);

    // Subtract number of samples pending in our buffer and not copied to resampler yet.
    // Now we have tail of input stream inside resampler.
    out_cts -= in_spec_.samples_overall_2_ns(in_buf_pos_);

    // Subtract number of input samples that resampler haven't processed yet.
    // Now we have point in input stream corresponding to tail of output frame.
    out_cts -= in_spec_.fract_samples_overall_2_ns(resampler_.n_left_to_process());

    // Subtract length of current output frame multiplied by scaling.
    // Now we have point in input stream corresponding to head of output frame.
    out_cts -=
        core::nanoseconds_t(out_spec_.samples_overall_2_ns(out_frame_pos_) * scaling_);

    if (out_cts < 0) {
        // Input frame cts was very close to zero (unix epoch), in this case we
        // avoid producing negative cts until it grows a bit.
        return 0;
    }

    return out_cts;
}

} // namespace audio
} // namespace roc
