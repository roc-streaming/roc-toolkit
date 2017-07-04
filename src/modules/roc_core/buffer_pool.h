/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/buffer_pool.h
//! @brief Buffer pool.

#ifndef ROC_CORE_BUFFER_POOL_H_
#define ROC_CORE_BUFFER_POOL_H_

#include "roc_core/pool.h"

namespace roc {
namespace core {

template <class T> class Buffer;

//! Buffer pool.
template <class T> class BufferPool : public Pool<Buffer<T> > {
public:
    //! Initialization.
    BufferPool(IAllocator& allocator, size_t buff_sz, size_t n_buffs)
        : Pool<Buffer<T> >(allocator, sizeof(Buffer<T>) + sizeof(T) * buff_sz, n_buffs)
        , buff_sz_(buff_sz) {
    }

    //! Get buffer size.
    size_t buffer_size() const {
        return buff_sz_;
    }

private:
    size_t buff_sz_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_POOL_H_
