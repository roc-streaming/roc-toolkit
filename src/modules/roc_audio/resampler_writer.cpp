/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_writer.h"
#include "roc_core/log.h"
#include "roc_core/macros.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

ResamplerWriter::ResamplerWriter(IWriter& writer,
                                 core::BufferPool<sample_t>& buffer_pool,
                                 core::IAllocator& allocator,
                                 const ResamplerConfig& config,
                                 packet::channel_mask_t channels)
    : resampler_(allocator, config, channels)
    , writer_(writer)
    , window_i_(0)
    , out_buff_(allocator)
    , out_buff_i_(0)
    , frame_size_(config.frame_size)
    , valid_(false) {
    if (!init_window_(buffer_pool)) {
        return;
    }
    valid_ = resampler_.valid();
    out_buff_.resize(frame_size_);
}

bool ResamplerWriter::valid() const {
    return resampler_.valid();
}

bool ResamplerWriter::set_scaling(float scaling) {
    return resampler_.set_scaling(scaling);
}

void ResamplerWriter::write(Frame& frame) {
    roc_panic_if_not(valid());
    size_t frame_i = 0;

    for (; window_i_ < window_[0].size() && frame_i < frame.size();
         ++window_i_, ++frame_i) {
        window_[0].data()[window_i_] = frame.data()[frame_i];
    }
    for (; window_i_ < window_[0].size() * 2 && frame_i < frame.size();
         ++window_i_, ++frame_i) {
        window_[1].data()[window_i_ - window_[0].size()] = frame.data()[frame_i];
    }
    while (frame_i < frame.size()) {
        for (; window_i_ < window_[0].size() * 3 && frame_i < frame.size();
             ++window_i_, ++frame_i) {
            window_[2].data()[window_i_ - window_[0].size() * 2] = frame.data()[frame_i];
        }
        // All three slices are full, resampling frame_size_ samples.
        if (window_i_ >= window_[0].size() * 3) {
            resampler_.renew_buffers(window_[0], window_[1], window_[2]);

            Frame out_frame(&out_buff_[0], out_buff_.size());
            while (resampler_.resample_buff(out_frame)) {
                writer_.write(out_frame);
            }

            window_i_ -= window_[0].size();
            core::Slice<sample_t> temp = window_[0];
            window_[0] = window_[1];
            window_[1] = window_[2];
            window_[2] = temp;
        }
    }
}

bool ResamplerWriter::init_window_(core::BufferPool<sample_t>& buffer_pool) {
    roc_log(LogDebug, "resampler writer: initializing window");

    if (buffer_pool.buffer_size() < frame_size_) {
        roc_log(LogError,
                "resampler writer: buffer size too small: required=%lu actual=%lu",
                (unsigned long)frame_size_, (unsigned long)buffer_pool.buffer_size());
        return false;
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(window_); n++) {
        window_[n] = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);

        if (!window_[n]) {
            roc_log(LogError, "resampler writer: can't allocate buffer");
            return false;
        }

        window_[n].resize(frame_size_);
    }
    return true;
}

} // namespace audio
} // namespace roc
