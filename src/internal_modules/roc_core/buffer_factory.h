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
#include "roc_core/buffer.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace core {

//! Buffer factory.
//! Allows to instantiate fixed-size buffers.
//! @tparam T define buffer element type.
template <class T> class BufferFactory : public core::NonCopyable<> {
public:
    //! Initialization.
    //! @p buffer_size defines number of elements in buffer.
    BufferFactory(IPool& pool, size_t buffer_size)
        : buffer_pool_(pool)
        , buffer_size_(buffer_size) {
    }

    //! Get number of elements in buffer.
    size_t buffer_size() const {
        return buffer_size_;
    }

    //! Allocate new buffer.
    SharedPtr<Buffer<T> > new_buffer() {
        return new (buffer_pool_) Buffer<T>(buffer_pool_, buffer_size_);
    }

private:
    IPool& buffer_pool_;
    const size_t buffer_size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_BUFFER_FACTORY_H_
