/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/buffer.h
//! @brief Buffer.

#ifndef ROC_CORE_BUFFER_H_
#define ROC_CORE_BUFFER_H_

#include "roc_core/buffer_pool.h"
#include "roc_core/refcnt.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Buffer.
template <class T> class Buffer : public RefCnt<Buffer<T> > {
public:
    //! Initialize empty buffer.
    explicit Buffer(BufferPool<T>& pool)
        : pool_(pool) {
        new (data()) T[size()];
    }

    //! Get buffer data.
    T* data() {
        return (T*)(((char*)this) + sizeof(Buffer));
    }

    //! Get maximum number of elements.
    size_t size() const {
        return pool_.buffer_size();
    }

    //! Get pointer to buffer from the pointer to its data.
    static Buffer* container_of(void* data) {
        return (Buffer*)((char*)data - sizeof(Buffer));
    }

private:
    friend class RefCnt<Buffer>;

    void destroy() {
        pool_.destroy(*this);
    }

    BufferPool<T>& pool_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_H_
