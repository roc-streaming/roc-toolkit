/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/default_buffer_composer.h
//! @brief Default IBufferComposer implementation.

#ifndef ROC_CORE_DEFAULT_BUFFER_COMPOSER_H_
#define ROC_CORE_DEFAULT_BUFFER_COMPOSER_H_

#include "roc_core/default_buffer.h"
#include "roc_core/heap_pool.h"
#include "roc_core/ibuffer_composer.h"
#include "roc_core/ipool.h"

namespace roc {
namespace core {

//! Default IBufferComposer implementation.
template <size_t MaxSz, class T, class Buffer = DefaultBuffer<MaxSz, T> >
class DefaultBufferComposer : public IBufferComposer<T> {
private:
    typedef SharedPtr<IBuffer<T> > IBufferPtr;

public:
    //! Initialize.
    explicit DefaultBufferComposer(IPool<Buffer>& pool = HeapPool<Buffer>::instance())
        : pool_(pool) {
    }

    //! Create buffer.
    virtual IBufferPtr compose() {
        return new (pool_) Buffer(pool_);
    }

    //! Restore existing buffer by pointer to its data.
    virtual IBufferPtr container_of(T* data) {
        return Buffer::container_of(data);
    }

private:
    IPool<Buffer>& pool_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_DEFAULT_BUFFER_COMPOSER_H_
