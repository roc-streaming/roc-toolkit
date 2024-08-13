/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/iarena.h
//! @brief Memory arena interface.

#ifndef ROC_CORE_IARENA_H_
#define ROC_CORE_IARENA_H_

#include "roc_core/attributes.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Memory arena interface.
class IArena {
public:
    virtual ~IArena();

    //! Allocate memory.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory at least of @p size
    //!  bytes or NULL if memory can't be allocated.
    virtual void* allocate(size_t size) = 0;

    //! Deallocate previously allocated memory.
    virtual void deallocate(void* ptr) = 0;

    //! Computes how many bytes will be actually allocated if allocate() is called with
    //! given size. Covers all internal overhead, if any.
    virtual size_t compute_allocated_size(size_t size) const = 0;

    //! Returns how many bytes was allocated for given pointer returned by allocate().
    //! Covers all internal overhead, if any.
    //! Returns same value as computed by compute_allocated_size(size).
    virtual size_t allocated_size(void* ptr) const = 0;

    //! Destroy object and deallocate its memory.
    template <class T> void dispose_object(T& object) {
        object.~T();
        deallocate(&object);
    }
};

} // namespace core
} // namespace roc

//! Placement new for core::IArena.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
inline void* operator new(size_t size, roc::core::IArena& arena) throw() {
    return arena.allocate(size);
}

//! Placement new[] for core::IArena.
//! @note
//!  nothrow forces compiler to check for NULL return value before calling ctor.
inline void* operator new[](size_t size, roc::core::IArena& arena) throw() {
    return arena.allocate(size);
}

//! Placement delete for core::IArena.
//! @note
//!  Compiler calls this if ctor throws in a placement new expression.
template <class T>
inline void operator delete(void* ptr, roc::core::IArena& arena) throw() {
    arena.deallocate(ptr);
}

//! Placement delete[] for core::IArena.
//! @note
//!  Compiler calls this if ctor throws in a placement new[] expression.
template <class T>
inline void operator delete[](void* ptr, roc::core::IArena& arena) throw() {
    arena.deallocate(ptr);
}

#endif // ROC_CORE_IARENA_H_
