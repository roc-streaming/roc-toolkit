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
    , input_pos_(0)
    , output_pos_(0)
    , scaling_(1.f)
    , next_scaling_(scaling_)
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

    if (!(output_ = buffer_factory.new_buffer())) {
        roc_log(LogError, "resampler writer: can't allocate buffer for output frame");
        return;
    }
    output_.reslice(0, output_.capacity());

    valid_ = true;
}

bool ResamplerWriter::is_valid() const {
    return valid_;
}

bool ResamplerWriter::set_scaling(float multiplier) {
    roc_panic_if_not(is_valid());

    scaling_ = next_scaling_ = multiplier;
    return resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                  out_sample_spec_.sample_rate(), multiplier);
}

void ResamplerWriter::write(Frame& frame) {
    roc_panic_if_not(is_valid());

    if (frame.num_samples() % in_sample_spec_.num_channels() != 0) {
        roc_panic("resampler writer: unexpected frame size");
    }

    size_t frame_pos = 0;

    while (frame_pos < frame.num_samples()) {
        Frame out_part(output_.data() + output_pos_, output_.size() - output_pos_);

        const size_t num_popped = resampler_.pop_output(out_part);

        if (num_popped < out_part.num_samples()) {
            frame_pos += push_input_(frame, frame_pos);
        }

        output_pos_ += num_popped;

        if (output_pos_ == output_.size()) {
            Frame out_frame(output_.data(), output_.size());
            out_frame.set_capture_timestamp(capture_ts_(frame, frame_pos));

            writer_.write(out_frame);

            output_pos_ = 0;
        }
    }

    if (output_pos_ != 0) {
        Frame out_frame(output_.data(), output_pos_);
        out_frame.set_capture_timestamp(capture_ts_(frame, frame_pos));

        scaling_ = next_scaling_;
        writer_.write(out_frame);

        output_pos_ = 0;
    }
}

size_t ResamplerWriter::push_input_(Frame& frame, size_t frame_pos) {
    if (input_pos_ == 0) {
        input_ = resampler_.begin_push_input();
    }

    const size_t num_copy =
        std::min(frame.num_samples() - frame_pos, input_.size() - input_pos_);

    memcpy(input_.data() + input_pos_, frame.samples() + frame_pos,
           num_copy * sizeof(sample_t));

    input_pos_ += num_copy;

    if (input_pos_ == input_.size()) {
        input_pos_ = 0;
        resampler_.end_push_input();
    }

    return num_copy;
}

core::nanoseconds_t ResamplerWriter::capture_ts_(Frame& frame, size_t frame_pos) {
    if (frame.capture_timestamp() == 0) {
        // we didn't receive frame with non-zero cts yet
        return 0;
    }

    const core::nanoseconds_t capt_ts = frame.capture_timestamp()
        + in_sample_spec_.samples_overall_2_ns(frame_pos)  // last added sample ts
        - in_sample_spec_.samples_overall_2_ns(input_pos_) // num unprocessed inside
        - in_sample_spec_.fract_samples_per_chan_2_ns(resampler_.n_left_to_process())
        - core::nanoseconds_t(out_sample_spec_.samples_overall_2_ns(output_pos_)
                              * scaling_);

    if (capt_ts < 0) {
        // frame cts was very close to zero (unix epoch), in this case we
        // avoid producing negative cts
        return 0;
    }

    return capt_ts;
}

} // namespace audio
} // namespace roc
