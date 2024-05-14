/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/buffer_factory.h
//! @brief Buffer factory.

#ifndef ROC_CORE_BUFFER_FACTORY_H_
#define ROC_CORE_BUFFER_FACTORY_H_

#include "roc_core/buffer.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slab_pool.h"

namespace roc {
namespace core {

//! Buffer factory.
//! Allows to instantiate fixed-size byte buffers.
class BufferFactory : public core::NonCopyable<> {
public:
    //! Initialization.
    //! @p buffer_size defines size in bytes of each buffer.
    BufferFactory(IArena& arena, size_t buffer_size);

    //! Get buffer size in bytes.
    size_t buffer_size() const;

    //! Allocate new buffer.
    BufferPtr new_buffer();

private:
    SlabPool<Buffer> buffer_pool_;
    const size_t buffer_size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_FACTORY_H_
