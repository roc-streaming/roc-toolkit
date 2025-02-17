/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ipool.h
//! @brief Memory pool interface.

#ifndef ROC_CORE_IPOOL_H_
#define ROC_CORE_IPOOL_H_

#include "roc_core/attributes.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory pool interface.
class IPool {
public:
    virtual ~IPool();

    //! Get size of the allocation per object.
    //! Covers all internal overhead, if any.
    virtual size_t allocation_size() const = 0;

    //! Get size of the object (without overhead).
    virtual size_t object_size() const = 0;

    //! Reserve memory for given number of objects.
    //! @returns
    //!  false if allocation failed.
    virtual ROC_ATTR_NODISCARD bool reserve(size_t n_objects) = 0;

    //! Allocate memory for an object.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory for a new object
    //!  or NULL if memory can't be allocated.
    virtual void* allocate() = 0;

    //! Return memory to pool.
    virtual void deallocate(void* memory) = 0;

    //! Destroy object and deallocate its memory.
    template <class T> void dispose_object(T& object) {
        object.~T();
        deallocate(&object);
    }
};

} // namespace core
} // namespace roc

//! Placement new for core::IPool.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
inline void* operator new(size_t size, roc::core::IPool& pool) throw() {
    roc_panic_if(pool.object_size() < size);
    return pool.allocate();
}

//! Placement delete for core::IPool.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
inline void operator delete(void* ptr, roc::core::IPool& pool) throw() {
    pool.deallocate(ptr);
}

#endif // ROC_CORE_IPOOL_H_
