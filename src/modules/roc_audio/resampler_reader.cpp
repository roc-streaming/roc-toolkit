/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_reader.h"
#include "roc_core/log.h"
#include "roc_core/macros.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

ResamplerReader::ResamplerReader(IReader& reader,
                                 core::BufferPool<sample_t>& buffer_pool,
                                 core::IAllocator& allocator,
                                 const ResamplerConfig& config,
                                 packet::channel_mask_t channels)
    : resampler_(allocator, config, channels)
    , reader_(reader)
    , frame_size_(config.frame_size)
    , buffers_empty_(true)
    , valid_(false) {
    if (!init_window_(buffer_pool)) {
        return;
    }
    valid_ = resampler_.valid();
}

bool ResamplerReader::valid() const {
    return resampler_.valid();
}

bool ResamplerReader::set_scaling(float scaling) {
    return resampler_.set_scaling(scaling);
}

void ResamplerReader::read(Frame& frame) {
    roc_panic_if_not(valid());

    if (buffers_empty_) {
        renew_window_();
    }

    while (!resampler_.resample_buff(frame)) {
        renew_window_();
    }
}

bool ResamplerReader::init_window_(core::BufferPool<sample_t>& buffer_pool) {
    roc_log(LogDebug, "resampler reader: initializing window");

    if (buffer_pool.buffer_size() < frame_size_) {
        roc_log(LogError,
                "resampler reader: buffer size too small: required=%lu actual=%lu",
                (unsigned long)frame_size_, (unsigned long)buffer_pool.buffer_size());
        return false;
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(window_); n++) {
        window_[n] = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);

        if (!window_[n]) {
            roc_log(LogError, "resampler reader: can't allocate buffer");
            return false;
        }

        window_[n].resize(frame_size_);
    }
    return true;
}

void ResamplerReader::renew_window_() {
    if (buffers_empty_) {
        for (size_t n = 0; n < ROC_ARRAY_SIZE(window_); ++n) {
            Frame frame(window_[n].data(), window_[n].size());
            reader_.read(frame);
        }
        buffers_empty_ = false;
    } else {
        core::Slice<sample_t> temp = window_[0];
        window_[0] = window_[1];
        window_[1] = window_[2];
        window_[2] = temp;

        Frame frame(window_[2].data(), window_[2].size());
        reader_.read(frame);
    }

    resampler_.renew_buffers(window_[0], window_[1], window_[2]);
}

} // namespace audio
} // namespace roc
