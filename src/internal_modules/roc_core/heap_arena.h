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
//! Supports memory "poisoning" to make memory-related bugs (out of bound writes, use
//! after free, etc) more noticeable.
//!
//! The memory is always maximum aligned. Thread-safe.
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
    virtual void deallocate(void*);

private:
    struct ChunkHeader {
        size_t size;
        AlignMax data[];
    };

    typedef AlignMax ChunkCanary;

    Atomic<int> num_allocations_;

    static size_t flags_;

    size_t num_guard_failures_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HEAP_ARENA_H_
