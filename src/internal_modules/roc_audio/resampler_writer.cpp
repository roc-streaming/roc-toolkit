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

ResamplerWriter::ResamplerWriter(IWriter& writer,
                                 IResampler& resampler,
                                 core::BufferFactory<sample_t>& buffer_factory,
                                 core::nanoseconds_t frame_length,
                                 const SampleSpec& in_sample_spec,
                                 const SampleSpec& out_sample_spec)
    : resampler_(resampler)
    , writer_(writer)
    , in_sample_spec_(in_sample_spec)
    , out_sample_spec_(out_sample_spec)
    , input_pos_(0)
    , output_pos_(0)
    , valid_(false) {
    if (!resampler_.valid()) {
        return;
    }

    if (!resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                out_sample_spec_.sample_rate(), 1.0f)) {
        return;
    }

    const size_t out_frame_size = out_sample_spec_.ns_2_samples_overall(frame_length);
    if (out_frame_size == 0) {
        roc_log(LogError, "resampler writer: frame size can't be zero");
        return;
    }

    if (!(output_ = buffer_factory.new_buffer())) {
        roc_log(LogError, "resampler writer: can't allocate buffer for output frame");
        return;
    }
    output_.reslice(0, out_frame_size);

    valid_ = true;
}

bool ResamplerWriter::valid() const {
    return valid_;
}

bool ResamplerWriter::set_scaling(float multiplier) {
    roc_panic_if_not(valid());

    return resampler_.set_scaling(in_sample_spec_.sample_rate(),
                                  out_sample_spec_.sample_rate(), multiplier);
}

void ResamplerWriter::write(Frame& frame) {
    roc_panic_if_not(valid());

    size_t frame_pos = 0;

    while (frame_pos < frame.size()) {
        Frame out_part(output_.data() + output_pos_, output_.size() - output_pos_);

        const size_t num_popped = resampler_.pop_output(out_part);

        if (num_popped < out_part.size()) {
            frame_pos += push_input_(frame, frame_pos);
        }

        output_pos_ += num_popped;

        if (output_pos_ == output_.size()) {
            output_pos_ = 0;

            Frame out_frame(output_.data(), output_.size());
            writer_.write(out_frame);
        }
    }
}

size_t ResamplerWriter::push_input_(Frame& frame, size_t frame_pos) {
    if (input_pos_ == 0) {
        input_ = resampler_.begin_push_input();
    }

    const size_t num_copy =
        std::min(frame.size() - frame_pos, input_.size() - input_pos_);

    memcpy(input_.data() + input_pos_, frame.data() + frame_pos,
           num_copy * sizeof(sample_t));

    input_pos_ += num_copy;

    if (input_pos_ == input_.size()) {
        input_pos_ = 0;
        resampler_.end_push_input();
    }

    return num_copy;
}

} // namespace audio
} // namespace roc
