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
                                 roc::packet::channel_mask_t ch_mask)
    : resampler_(resampler)
    , writer_(writer)
    , frame_size_(packet::ns_to_size(frame_length, sample_rate, ch_mask))
    , frame_pos_(0)
    , valid_(false) {
    if (!resampler_.valid()) {
        return;
    }
    if (frame_size_ == 0) {
        return;
    }
    if (!init_(buffer_pool)) {
        return;
    }
    valid_ = true;
}

bool ResamplerWriter::valid() const {
    return valid_;
}

bool ResamplerWriter::set_scaling(size_t input_sample_rate,
                                  size_t output_sample_rate,
                                  float multiplier) {
    roc_panic_if_not(valid());

    return resampler_.set_scaling(input_sample_rate, output_sample_rate, multiplier);
}

void ResamplerWriter::write(Frame& input) {
    roc_panic_if_not(valid());

    const sample_t* input_data = input.data();
    const size_t input_size = input.size();
    size_t input_pos = 0;

    sample_t* frame0_data = frames_[0].data();
    for (; frame_pos_ < frame_size_ && input_pos < input_size;
         ++frame_pos_, ++input_pos) {
        frame0_data[frame_pos_] = input_data[input_pos];
    }

    sample_t* frame1_data = frames_[1].data();
    for (; frame_pos_ < frame_size_ * 2 && input_pos < input_size;
         ++frame_pos_, ++input_pos) {
        frame1_data[frame_pos_ - frame_size_] = input_data[input_pos];
    }

    while (input_pos < input_size) {
        sample_t* frame2_data = frames_[2].data();
        for (; frame_pos_ < frame_size_ * 3 && input_pos < input_size;
             ++frame_pos_, ++input_pos) {
            frame2_data[frame_pos_ - frame_size_ * 2] = input_data[input_pos];
        }

        // All three slices are full, resampling frame_size_ samples.
        if (frame_pos_ >= frame_size_ * 3) {
            resampler_.renew_buffers(frames_[0], frames_[1], frames_[2]);

            Frame out_frame(output_.data(), output_.size());
            while (resampler_.resample_buff(out_frame)) {
                writer_.write(out_frame);
            }

            frame_pos_ -= frames_[0].size();
            core::Slice<sample_t> temp = frames_[0];
            frames_[0] = frames_[1];
            frames_[1] = frames_[2];
            frames_[2] = temp;
        }
    }
}

bool ResamplerWriter::init_(core::BufferPool<sample_t>& buffer_pool) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(frames_); n++) {
        frames_[n] = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);

        if (!frames_[n]) {
            roc_log(LogError, "resampler writer: can't allocate buffer");
            return false;
        }

        frames_[n].resize(frame_size_);
    }

    output_ = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);

    if (!output_) {
        roc_log(LogError, "resampler writer: can't allocate buffer");
        return false;
    }

    output_.resize(frame_size_);

    return true;
}

} // namespace audio
} // namespace roc
