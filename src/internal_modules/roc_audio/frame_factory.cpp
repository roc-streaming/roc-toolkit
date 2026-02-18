/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/frame_factory.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

FrameFactory::FrameFactory(core::IArena& arena, size_t buffer_size) {
    default_frame_pool_.reset(new (default_frame_pool_)
                                  core::SlabPool<Frame>("default_frame_pool", arena));
    default_buffer_pool_.reset(new (default_buffer_pool_) core::SlabPool<core::Buffer>(
        "default_frame_buffer_pool", arena, sizeof(core::Buffer) + buffer_size));

    frame_pool_ = default_frame_pool_.get();
    buffer_pool_ = default_buffer_pool_.get();
    buffer_size_ = buffer_size;
}

FrameFactory::FrameFactory(core::IPool& frame_pool, core::IPool& buffer_pool) {
    if (frame_pool.object_size() != sizeof(Frame)) {
        roc_panic("frame factory: unexpected frame_pool object size:"
                  " expected=%lu actual=%lu",
                  (unsigned long)sizeof(Frame), (unsigned long)frame_pool.object_size());
    }

    if (buffer_pool.object_size() < sizeof(core::Buffer)) {
        roc_panic("frame factory: unexpected buffer_pool object size:"
                  " minimum=%lu actual=%lu",
                  (unsigned long)sizeof(core::Buffer),
                  (unsigned long)buffer_pool.object_size());
    }

    frame_pool_ = &frame_pool;
    buffer_pool_ = &buffer_pool;
    buffer_size_ = buffer_pool.object_size() - sizeof(core::Buffer);
}

size_t FrameFactory::byte_buffer_size() const {
    return buffer_size_;
}

core::Slice<uint8_t> FrameFactory::new_byte_buffer() {
    core::BufferPtr buf = new (*buffer_pool_) core::Buffer(*buffer_pool_, buffer_size_);
    if (!buf) {
        roc_log(LogError, "frame factory: failed to allocate byte buffer");
    }

    return buf;
}

size_t FrameFactory::raw_buffer_size() const {
    return buffer_size_ / sizeof(sample_t);
}

core::Slice<sample_t> FrameFactory::new_raw_buffer() {
    core::BufferPtr buf = new (*buffer_pool_) core::Buffer(*buffer_pool_, buffer_size_);
    if (!buf) {
        roc_log(LogError, "frame factory: failed to allocate raw buffer");
    }

    return buf;
}

FramePtr FrameFactory::allocate_frame_no_buffer() {
    FramePtr frame = new (*frame_pool_) Frame(*frame_pool_);
    if (!frame) {
        roc_log(LogError, "frame factory: failed to allocate frame");
    }

    return frame;
}

FramePtr FrameFactory::allocate_frame(size_t n_bytes) {
    if (n_bytes > buffer_size_) {
        roc_log(LogError,
                "frame factory: requested buffer size is too large:"
                " requested=%lu maximum=%lu",
                (unsigned long)n_bytes, (unsigned long)buffer_size_);
        return NULL;
    }

    FramePtr frame = allocate_frame_no_buffer();
    if (!frame) {
        return NULL;
    }

    core::Slice<uint8_t> buf = new_byte_buffer();
    if (!buf) {
        return NULL;
    }

    buf.reslice(0, n_bytes);
    frame->set_buffer(buf);

    return frame;
}

bool FrameFactory::reallocate_frame(Frame& frame, size_t n_bytes) {
    core::Slice<uint8_t> buf = frame.buffer();

    frame.clear();

    if (!buf || (n_bytes > buf.capacity() && n_bytes <= buffer_size_)) {
        if (!(buf = new_byte_buffer())) {
            return false;
        }
    }

    if (n_bytes > buf.capacity()) {
        roc_log(LogError,
                "frame factory: requested buffer size is too large:"
                " requested=%lu maximum=%lu",
                (unsigned long)n_bytes, (unsigned long)buf.capacity());
        return false;
    }

    buf.reslice(0, n_bytes);
    frame.set_buffer(buf);

    return true;
}

} // namespace audio
} // namespace roc
