/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/buffer.h
//! @brief Buffer.

#ifndef ROC_CORE_BUFFER_H_
#define ROC_CORE_BUFFER_H_

#include "roc_core/align_ops.h"
#include "roc_core/allocation_policy.h"
#include "roc_core/ipool.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

class Buffer;

//! Buffer smart pointer.
typedef SharedPtr<Buffer> BufferPtr;

//! Fixed-size dynamically-allocated byte buffer.
//!
//! @remarks
//!  Buffer is typically allocated from pool and then used to create a Slice.
//!  Slice holds a shared pointer to either Buffer or BufferView and implements
//!  type-safety and dynamic resizing on top of it.
//!  Slices are widely used to hold data of packets and frames.
//!
//! @note
//!  Buffer has a reference counter. When it reaches zero (i.e. when there are
//!  no slices referring buffer), buffer is returned to pool.
//!
//! @note
//!  Buffer size is fixed, but determined at runtime, not compile time.
//!  It is defined by the pool that allocates the buffer.
//!
//! @see BufferView, Slice.
class Buffer : public RefCounted<Buffer, PoolAllocation> {
public:
    //! Initialize empty buffer.
    Buffer(IPool& buffer_pool, size_t buffer_size);

    //! Get buffer size in bytes.
    size_t size() const {
        return size_;
    }

    //! Get buffer data.
    uint8_t* data() {
        return (uint8_t*)data_;
    }

    //! Get pointer to buffer from the pointer to its data.
    static Buffer* container_of(void* data) {
        return ROC_CONTAINER_OF(data, Buffer, data_);
    }

private:
    const size_t size_;
    AlignMax data_[];
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_H_
