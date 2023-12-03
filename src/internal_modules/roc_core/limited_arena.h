/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/limited_arena.h
//! @brief Limited Arena.

#ifndef ROC_CORE_LIMITED_ARENA_H_
#define ROC_CORE_LIMITED_ARENA_H_

#include "roc_core/iarena.h"
#include "roc_core/memory_limiter.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Decorator around IArena to make it memory limited.
class LimitedArena : public NonCopyable<>, public IArena {
public:
    //! Initialize.
    LimitedArena(IArena& arena, MemoryLimiter& memory_limiter);

    //! Allocate memory after checking with the memory limiter.
    //! @returns
    //!  pointer to a maximum aligned uninitialized memory at least of @p size
    //!  bytes or NULL if memory can't be allocated.
    virtual void* allocate(size_t size);

    //! Deallocate previously allocated memory.
    virtual void deallocate(void* ptr);

    //! Computes how many bytes will be actually allocated if allocate() is called with
    //! given size. Covers all internal overhead, if any.
    virtual size_t compute_allocated_size(size_t size) const;

    //! Returns how many bytes was allocated for given pointer returned by allocate().
    //! Covers all internal overhead, if any.
    //! Returns same value as computed by compute_allocated_size(size).
    virtual size_t allocated_size(void* ptr) const;

private:
    IArena& arena_;
    MemoryLimiter& memory_limiter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIMITED_ARENA_H_
