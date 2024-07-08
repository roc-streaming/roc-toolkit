/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/buffer_view.h
//! @brief Buffer view.

#ifndef ROC_CORE_BUFFER_VIEW_H_
#define ROC_CORE_BUFFER_VIEW_H_

#include "roc_core/allocation_policy.h"
#include "roc_core/ipool.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

class BufferView;

//! Buffer view smart pointer.
typedef SharedPtr<BufferView> BufferViewPtr;

//! Fixed-size byte buffer view.
//!
//! @remarks
//!  BufferView points to a fixed-size memory chunk that it doesn't own.
//!  BufferView is typically used to construct a Slice.
//!  Slice holds a shared pointer to either Buffer or BufferView and implements
//!  type-safety and dynamic resizing on top of it.
//!  Slices are widely used to hold data of packets and frames.
//!
//! @note
//!  BufferView has a reference counter used for lifetime checks.
//!  When it reaches zero, nothing actually happens. However, when view
//!  destructor is called, it panics if reference counter is non-zero
//!  (i.e. if there are still slices referring to it).
//!
//! @see Buffer, Slice.
class BufferView : public RefCounted<BufferView, NoopAllocation> {
public:
    //! Initialize view referring to memory region.
    //! Memory should remain valid until view in destroyed.
    BufferView(void* data, size_t size);

    //! Get memory size in bytes.
    size_t size() const {
        return size_;
    }

    //! Get memory.
    uint8_t* data() {
        return (uint8_t*)data_;
    }

private:
    const size_t size_;
    void* data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_VIEW_H_
