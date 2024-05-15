/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/frame_factory.h
//! @brief Frame factory.

#ifndef ROC_AUDIO_FRAME_FACTORY_H_
#define ROC_AUDIO_FRAME_FACTORY_H_

#include "roc_audio/frame.h"
#include "roc_audio/sample.h"
#include "roc_core/buffer.h"
#include "roc_core/iarena.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/slab_pool.h"
#include "roc_core/slice.h"

namespace roc {
namespace audio {

//! Frame factory.
//!
//! Allows allocating frames and frame buffers (byte buffers of appropriate size
//! that should be attached to frame to hold payload).
//!
//! Serves several purposes:
//!  - implements convenient and type-safe wrapper on top of memory pools
//!  - combines two related pools (frame pool and buffer pool) in one class
//!  - detaches pipeline logic from memory management interface, so that it can
//!    change independently without affecting every pipeline element
class FrameFactory : public core::NonCopyable<> {
public:
    //! Initialize with default pools.
    //! @p buffer_size defines number of bytes in frame buffer.
    FrameFactory(core::IArena& arena, size_t buffer_size);

    //! Initialize with custom pools.
    //! @p buffer_pool is a pool of core::Buffer objects.
    FrameFactory(core::IPool& buffer_pool);

    //! Get number of bytes in byte buffer.
    size_t byte_buffer_size() const;

    //! Allocate byte buffer.
    core::Slice<uint8_t> new_byte_buffer();

    //! Get number of samples in raw sample buffer.
    size_t raw_buffer_size() const;

    //! Allocate raw sample buffer.
    core::Slice<sample_t> new_raw_buffer();

private:
    // used if factory is created with default pools
    core::Optional<core::SlabPool<core::Buffer> > default_buffer_pool_;

    core::IPool* buffer_pool_;
    size_t buffer_size_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FRAME_FACTORY_H_
