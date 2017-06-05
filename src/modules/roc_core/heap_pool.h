/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/heap_pool.h
//! @brief IPool implementation using heap.

#ifndef ROC_CORE_HEAP_POOL_H_
#define ROC_CORE_HEAP_POOL_H_

#include "roc_core/atomic.h"
#include "roc_core/ipool.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/singleton.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! IPool implementation using heap.
template <class T> class HeapPool : public IPool<T>, public NonCopyable<> {
public:
    ~HeapPool() {
        if (num_allocated_ != 0) {
            roc_panic("memory leak in heap pool: %u leaked elements",
                      (unsigned)num_allocated_);
        }
    }

    //! Allocate memory for new object.
    virtual void* allocate() {
        // Note: `new char[]` returns memory aligned for any type.
        void* memory = new (std::nothrow) char[sizeof(T)];
        if (memory != NULL) {
            ++num_allocated_;
        }
        return memory;
    }

    //! Destroy previously allocated object and memory.
    virtual void deallocate(void* memory) {
        roc_panic_if(memory == NULL);
        if (num_allocated_ == 0) {
            roc_panic(
                "trying to deallocate more objects than were allocated in heap pool");
        }
        --num_allocated_;
        delete[](char*) memory;
    }

    //! Check if this object belongs to this pool.
    virtual void check(T& object) {
        roc_panic_if((const void*)&object == NULL);
    }

    //! Get static instance.
    static HeapPool& instance() {
        return Singleton<HeapPool>::instance();
    }

private:
    Atomic num_allocated_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HEAP_POOL_H_
