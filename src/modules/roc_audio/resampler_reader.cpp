/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_reader.h"
#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

ResamplerReader::ResamplerReader(IReader& reader,
                                 IResampler& resampler,
                                 core::BufferPool<sample_t>& buffer_pool,
                                 core::nanoseconds_t frame_length,
                                 size_t sample_rate,
                                 roc::packet::channel_mask_t ch_mask)
    : resampler_(resampler)
    , reader_(reader)
    , frame_size_(packet::ns_to_size(frame_length, sample_rate, ch_mask))
    , frames_empty_(true)
    , valid_(false) {
    if (!resampler_.valid()) {
        return;
    }

    if (frame_size_ == 0) {
        return;
    }
    if (!init_frames_(buffer_pool)) {
        return;
    }

    valid_ = true;
}

bool ResamplerReader::valid() const {
    return valid_;
}

bool ResamplerReader::set_scaling(size_t input_sample_rate,
                                  size_t output_sample_rate,
                                  float multiplier) {
    roc_panic_if_not(valid());

    return resampler_.set_scaling(input_sample_rate, output_sample_rate, multiplier);
}

bool ResamplerReader::read(Frame& frame) {
    roc_panic_if_not(valid());

    if (frames_empty_) {
        renew_frames_();
    }

    while (!resampler_.resample_buff(frame)) {
        if (!renew_frames_()) {
            return false;
        }
    }

    return true;
}

bool ResamplerReader::init_frames_(core::BufferPool<sample_t>& buffer_pool) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(frames_); n++) {
        frames_[n] = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);

        if (!frames_[n]) {
            roc_log(LogError, "resampler reader: can't allocate buffer");
            return false;
        }

        frames_[n].resize(frame_size_);
    }

    return true;
}

bool ResamplerReader::renew_frames_() {
    if (frames_empty_) {
        for (size_t n = 0; n < ROC_ARRAY_SIZE(frames_); ++n) {
            Frame frame(frames_[n].data(), frames_[n].size());
            if (!reader_.read(frame)) {
                return false;
            }
        }
        frames_empty_ = false;
    } else {
        core::Slice<sample_t> temp = frames_[0];
        frames_[0] = frames_[1];
        frames_[1] = frames_[2];
        frames_[2] = temp;

        Frame frame(frames_[2].data(), frames_[2].size());
        if (!reader_.read(frame)) {
            return false;
        }
    }

    resampler_.renew_buffers(frames_[0], frames_[1], frames_[2]);
    return true;
}

} // namespace audio
} // namespace roc
