/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ipool.h
//! @brief Pool interface.

#ifndef ROC_CORE_IPOOL_H_
#define ROC_CORE_IPOOL_H_

#include "roc_core/attributes.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Pool interface.
template <class T> class IPool {
public:
    virtual ~IPool() {
    }

    //! Allocate memory for new object.
    //! @returns
    //!  properly aligned uninitialized memory at least of sizeof(T)
    //!  bytes or NULL if memory can't be allocated.
    virtual void* allocate() = 0;

    //! Deallocate previously allocated memory.
    virtual void deallocate(void*) = 0;

    //! Destroy object and deallocate memory.
    void destroy(T& object) {
        check(object);
        object.~T();
        deallocate(&object);
    }

    //! Check if object belongs to this pool and isn't corrupted.
    //! @remarks
    //!  Optional. May call roc_panic() if some checks failed.
    virtual void check(T& object) = 0;
};

} // namespace core
} // namespace roc

//! Placement new for core::IPool<T>.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
template <class T>
inline void* operator new(size_t size, roc::core::IPool<T>& pool) ROC_ATTR_NOTHROW {
    using namespace roc;
    roc_panic_if(size != sizeof(T));
    return pool.allocate();
}

//! Placement delete for core::IPool<T>.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
template <class T>
inline void operator delete(void* ptr, roc::core::IPool<T>& pool)ROC_ATTR_NOTHROW {
    using namespace roc;
    pool.deallocate(ptr);
}

#endif // ROC_CORE_IPOOL_H_
