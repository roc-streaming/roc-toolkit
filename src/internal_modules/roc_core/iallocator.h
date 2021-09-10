/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/iallocator.h
//! @brief Memory allocator interface.

#ifndef ROC_CORE_IALLOCATOR_H_
#define ROC_CORE_IALLOCATOR_H_

#include "roc_core/attributes.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory allocator interface.
class IAllocator {
public:
    virtual ~IAllocator();

    //! Allocate memory.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory at least of @p size
    //!  bytes or NULL if memory can't be allocated.
    virtual void* allocate(size_t size) = 0;

    //! Deallocate previously allocated memory.
    virtual void deallocate(void*) = 0;

    //! Destroy object and deallocate its memory.
    template <class T> void destroy(T& object) {
        object.~T();
        deallocate(&object);
    }
};

} // namespace core
} // namespace roc

//! Placement new for core::IAllocator.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
inline void* operator new(size_t size, roc::core::IAllocator& allocator) throw() {
    return allocator.allocate(size);
}

//! Placement new[] for core::IAllocator.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
inline void* operator new[](size_t size, roc::core::IAllocator& allocator) throw() {
    return allocator.allocate(size);
}

//! Placement delete for core::IAllocator.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
template <class T>
inline void operator delete(void* ptr, roc::core::IAllocator& allocator) throw() {
    allocator.deallocate(ptr);
}

//! Placement delete[] for core::IAllocator.
//! @note
//!  Compiler calls this if ctor throws in a placement new[] expression.
template <class T>
inline void operator delete[](void* ptr, roc::core::IAllocator& allocator) throw() {
    allocator.deallocate(ptr);
}

#endif // ROC_CORE_IALLOCATOR_H_
