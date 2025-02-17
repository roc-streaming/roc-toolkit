/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/limited_pool.h
//! @brief Limited Pool.

#ifndef ROC_CORE_LIMITED_POOL_H_
#define ROC_CORE_LIMITED_POOL_H_

#include "roc_core/ipool.h"
#include "roc_core/memory_limiter.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Decorator around IPool to make it memory limited.
class LimitedPool : public NonCopyable<LimitedPool>, public IPool {
public:
    //! Initialize.
    LimitedPool(IPool& pool, MemoryLimiter& memory_limiter);

    //! Get size of the allocation per object.
    virtual size_t allocation_size() const;

    //! Get size of the object.
    virtual size_t object_size() const;

    //! Reserve memory for given number of objects.
    //! @returns
    //!  false if allocation failed.
    virtual ROC_ATTR_NODISCARD bool reserve(size_t n_objects);

    //! Allocate memory for an object, after checking with the memory limiter.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory for a new object
    //!  or NULL if memory can't be allocated.
    virtual void* allocate();

    //! Return memory to pool, then update the memory limiter.
    virtual void deallocate(void* memory);

private:
    IPool& pool_;
    MemoryLimiter& memory_limiter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIMITED_POOL_H_
