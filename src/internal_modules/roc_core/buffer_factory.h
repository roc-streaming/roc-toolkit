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

#include "roc_core/allocation_policy.h"
#include "roc_core/noncopyable.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/slab_pool.h"

namespace roc {
namespace core {

template <class T> class Buffer;

//! Buffer factory.
template <class T> class BufferFactory : public core::NonCopyable<> {
public:
    //! Initialization.
    BufferFactory(IAllocator& allocator, size_t buff_size, bool poison)
        : pool_(allocator, sizeof(Buffer<T>) + sizeof(T) * buff_size, poison)
        , buff_size_(buff_size) {
    }

    //! Get buffer size (number of elements in buffer).
    size_t buffer_size() const {
        return buff_size_;
    }

    //! Allocate new buffer.
    SharedPtr<Buffer<T> > new_buffer() {
        return new (pool_) Buffer<T>(*this);
    }

private:
    friend class FactoryAllocation<BufferFactory>;

    void destroy(Buffer<T>& buffer) {
        pool_.destroy_object(buffer);
    }

    SlabPool<> pool_;
    size_t buff_size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_FACTORY_H_
