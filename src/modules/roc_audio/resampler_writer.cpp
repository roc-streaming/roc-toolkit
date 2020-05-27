/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_writer.h"
#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

ResamplerWriter::ResamplerWriter(IWriter& writer,
                                 IResampler& resampler,
                                 core::BufferPool<sample_t>& buffer_pool,
                                 core::nanoseconds_t frame_length,
                                 size_t sample_rate,
                                 packet::channel_mask_t channels)
    : resampler_(resampler)
    , writer_(writer)
    , input_pos_(0)
    , output_pos_(0)
    , valid_(false) {
    if (!resampler_.valid()) {
        return;
    }

    const size_t frame_size = packet::ns_to_size(frame_length, sample_rate, channels);
    if (frame_size == 0) {
        roc_log(LogError, "resampler writer: frame size can't be zero");
        return;
    }

    if (!(output_ = new (buffer_pool) core::Buffer<sample_t>(buffer_pool))) {
        roc_log(LogError, "resampler writer: can't allocate buffer for output frame");
        return;
    }
    output_.resize(frame_size);

    valid_ = true;
}

bool ResamplerWriter::valid() const {
    return resampler_.valid();
}

bool ResamplerWriter::set_scaling(size_t input_sample_rate,
                                  size_t output_sample_rate,
                                  float multiplier) {
    roc_panic_if_not(valid());

    return resampler_.set_scaling(input_sample_rate, output_sample_rate, multiplier);
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
