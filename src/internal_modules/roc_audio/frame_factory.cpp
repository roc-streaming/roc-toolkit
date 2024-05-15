/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/frame_factory.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

FrameFactory::FrameFactory(core::IArena& arena, size_t buffer_size) {
    default_buffer_pool_.reset(new (default_buffer_pool_) core::SlabPool<core::Buffer>(
        "default_frame_buffer_pool", arena, sizeof(core::Buffer) + buffer_size));

    buffer_pool_ = default_buffer_pool_.get();
    buffer_size_ = buffer_size;
}

FrameFactory::FrameFactory(core::IPool& buffer_pool) {
    if (buffer_pool.object_size() < sizeof(core::Buffer)) {
        roc_panic("frame factory: unexpected buffer_pool object size:"
                  " minimum=%lu actual=%lu",
                  (unsigned long)sizeof(core::Buffer),
                  (unsigned long)buffer_pool.object_size());
    }

    buffer_pool_ = &buffer_pool;
    buffer_size_ = buffer_pool.object_size() - sizeof(core::Buffer);
}

size_t FrameFactory::byte_buffer_size() const {
    return buffer_size_;
}

core::Slice<uint8_t> FrameFactory::new_byte_buffer() {
    return (core::BufferPtr) new (*buffer_pool_)
        core::Buffer(*buffer_pool_, buffer_size_);
}

size_t FrameFactory::raw_buffer_size() const {
    return buffer_size_ / sizeof(sample_t);
}

core::Slice<sample_t> FrameFactory::new_raw_buffer() {
    return (core::BufferPtr) new (*buffer_pool_)
        core::Buffer(*buffer_pool_, buffer_size_);
}

} // namespace audio
} // namespace roc
