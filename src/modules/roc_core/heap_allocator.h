/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/heap_allocator.h
//! @brief Heap allocator implementation.

#ifndef ROC_CORE_HEAP_ALLOCATOR_H_
#define ROC_CORE_HEAP_ALLOCATOR_H_

#include "roc_core/atomic.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Heap allocator implementation.
//!
//! Uses global operator new[] and operator delete[].
//!
//! The memory is always maximum aligned. Thread-safe.
class HeapAllocator : public IAllocator, public NonCopyable<> {
public:
    //! Enable panic on leak in destructor, for all instances.
    static void enable_panic_on_leak();

    HeapAllocator();
    ~HeapAllocator();

    //! Get number of allocated blocks.
    size_t num_allocations() const;

    //! Allocate memory.
    virtual void* allocate(size_t size);

    //! Deallocate previously allocated memory.
    virtual void deallocate(void*);

private:
    static int panic_on_leak_;

    Atomic<int> num_allocations_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HEAP_ALLOCATOR_H_
