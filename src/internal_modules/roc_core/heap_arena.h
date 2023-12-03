/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/heap_arena.h
//! @brief Heap arena implementation.

#ifndef ROC_CORE_HEAP_ARENA_H_
#define ROC_CORE_HEAP_ARENA_H_

#include "roc_core/align_ops.h"
#include "roc_core/atomic.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Heap arena flags.
enum HeapArenaFlags {
    //! Enable panic if leaks detected in arena destructor.
    HeapArenaFlag_EnableLeakDetection = (1 << 0),
    //! Enable panic if memory violation detected when deallocating chunk.
    HeapArenaFlag_EnableGuards = (1 << 1),
};

//! Default heap arena flags.
enum { DefaultHeapArenaFlags = (HeapArenaFlag_EnableGuards) };

//! Heap arena implementation.
//!
//! Uses malloc() and free().
//!
//! The memory is always maximum aligned.
//!
//! Implements three safety measures:
//!  - to catch double-free and other logical bugs, inserts link to owning arena before
//!    user data, and panics if it differs when memory is returned to arena
//!  - to catch buffer overflow bugs, inserts "canary guards" before and after user
//!    data, and panics if they are overwritten when memory is returned to arena
//!  - to catch uninitialized-access and use-after-free bugs, "poisons" memory when it
//!    returned to user, and when it returned back to the arena
//!
//! Allocated chunks have the following format:
//! @code
//!  +-------------+-------------+-----------+-------------+
//!  | ChunkHeader | ChunkCanary | user data | ChunkCanary |
//!  +-------------+-------------+-----------+-------------+
//! @endcode
//!
//! ChunkHeader contains pointer to the owning arena, checked when returning memory to
//! arena. ChunkCanary contains magic bytes filled when returning memory to user, and
//! checked when returning memory to arena.
//!
//! Thread-safe.
class HeapArena : public IArena, public NonCopyable<> {
public:
    //! Initialize.
    HeapArena();
    ~HeapArena();

    //! Set flags, for all instances.
    //!
    //! @b Parameters
    //! - @p flags defines options to modify behaviour as indicated in HeapArenaFlags
    static void set_flags(size_t flags);

    //! Get number of allocated blocks.
    size_t num_allocations() const;

    //! Get number of guard failures.
    size_t num_guard_failures() const;

    //! Allocate memory.
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
    struct ChunkHeader {
        // The heap arena that the chunk belongs to.
        HeapArena* owner;
        // Data size, excluding canary guards.
        size_t size;
        // Data surrounded with canary guards.
        AlignMax data[];
    };

    typedef AlignMax ChunkCanary;

    Atomic<int> num_allocations_;
    Atomic<size_t> num_guard_failures_;

    static size_t flags_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HEAP_ARENA_H_
