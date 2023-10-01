/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_writer.h"
#include "roc_core/log.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

ResamplerWriter::ResamplerWriter(IFrameWriter& writer,
                                 IResampler& resampler,
                                 core::BufferFactory<sample_t>& buffer_factory,
                                 const SampleSpec& in_sample_spec,
                                 const SampleSpec& out_sample_spec)
    : resampler_(resampler)
    , writer_(writer)
    , in_sample_spec_(in_sample_spec)
    , out_sample_spec_(out_sample_spec)
    , input_buf_pos_(0)
    , output_buf_pos_(0)
    , scaling_(1.f)
    , valid_(false) {
    if (in_sample_spec_.channel_set() != out_sample_spec_.channel_set()) {
        roc_panic("resampler writer: input and output channel sets should be same");
    }

    if (!resampler_.is_valid()) {
        return;
    }

    if (!resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                out_sample_spec_.sample_rate(), 1.0f)) {
        return;
    }

    if (!(output_buf_ = buffer_factory.new_buffer())) {
        roc_log(LogError, "resampler writer: can't allocate buffer for output frame");
        return;
    }
    output_buf_.reslice(0, output_buf_.capacity());

    valid_ = true;
}

bool ResamplerWriter::is_valid() const {
    return valid_;
}

bool ResamplerWriter::set_scaling(float multiplier) {
    roc_panic_if_not(is_valid());

    scaling_ = multiplier;

    return resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                  out_sample_spec_.sample_rate(), multiplier);
}

void ResamplerWriter::write(Frame& in_frame) {
    roc_panic_if_not(is_valid());

    if (in_frame.num_samples() % in_sample_spec_.num_channels() != 0) {
        roc_panic("resampler writer: unexpected frame size");
    }

    size_t in_pos = 0;

    while (in_pos < in_frame.num_samples()) {
        const size_t output_buf_remain = output_buf_.size() - output_buf_pos_;

        const size_t num_popped = resampler_.pop_output(
            output_buf_.data() + output_buf_pos_, output_buf_remain);

        if (num_popped < output_buf_remain) {
            in_pos += push_input_(in_frame, in_pos);
        }

        output_buf_pos_ += num_popped;

        if (output_buf_pos_ == output_buf_.size()) {
            Frame out_frame(output_buf_.data(), output_buf_.size());
            out_frame.set_capture_timestamp(capture_ts_(in_frame, in_pos));

            writer_.write(out_frame);

            output_buf_pos_ = 0;
        }
    }

    if (output_buf_pos_ != 0) {
        Frame out_frame(output_buf_.data(), output_buf_pos_);
        out_frame.set_capture_timestamp(capture_ts_(in_frame, in_pos));

        writer_.write(out_frame);

        output_buf_pos_ = 0;
    }
}

size_t ResamplerWriter::push_input_(Frame& in_frame, size_t in_pos) {
    if (input_buf_pos_ == 0) {
        input_buf_ = resampler_.begin_push_input();
    }

    const size_t num_copy =
        std::min(in_frame.num_samples() - in_pos, input_buf_.size() - input_buf_pos_);

    memcpy(input_buf_.data() + input_buf_pos_, in_frame.samples() + in_pos,
           num_copy * sizeof(sample_t));

    input_buf_pos_ += num_copy;

    if (input_buf_pos_ == input_buf_.size()) {
        input_buf_pos_ = 0;
        resampler_.end_push_input();
    }

    return num_copy;
}

// Compute timestamp of first sample of current output frame.
// We have timestamps in input frames, and we should find to
// which time our output frame does correspond in input stream.
core::nanoseconds_t ResamplerWriter::capture_ts_(Frame& in_frame, size_t in_pos) {
    if (in_frame.capture_timestamp() == 0) {
        // We didn't receive input frame with non-zero cts yet,
        // so for now we keep cts zero.
        return 0;
    }

    // Get timestamp of first sample of last input frame pushed to resampler.
    core::nanoseconds_t out_cts = in_frame.capture_timestamp();

    // Add number of samples copied from input frame to our buffer and then to resampler.
    // Now we have tail of input stream.
    out_cts += in_sample_spec_.samples_overall_2_ns(in_pos);

    // Subtract number of samples pending in our buffer and not copied to resampler yet.
    // Now we have tail of input stream inside resampler.
    out_cts -= in_sample_spec_.samples_overall_2_ns(input_buf_pos_);

    // Subtract number of input samples that resampler haven't processed yet.
    // Now we have point in input stream corresponding to tail of output frame.
    out_cts -= in_sample_spec_.fract_samples_overall_2_ns(resampler_.n_left_to_process());

    // Subtract length of current output frame multiplied by scaling.
    // Now we have point in input stream corresponding to head of output frame.
    out_cts -= core::nanoseconds_t(out_sample_spec_.samples_overall_2_ns(output_buf_pos_)
                                   * scaling_);

    if (out_cts < 0) {
        // Input frame cts was very close to zero (unix epoch), in this case we
        // avoid producing negative cts until it grows a bit.
        return 0;
    }

    return out_cts;
}

} // namespace audio
} // namespace roc
