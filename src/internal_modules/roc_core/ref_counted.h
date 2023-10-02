/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ref_counted.h
//! @brief Base class for object with reference counter.

#ifndef ROC_CORE_REF_COUNTED_H_
#define ROC_CORE_REF_COUNTED_H_

#include "roc_core/allocation_policy.h"
#include "roc_core/atomic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/ref_counted_impl.h"

namespace roc {
namespace core {

//! Base class for object with reference counter.
//!
//! Allows to increment and decrement reference counter. When the counter
//! reaches zero, the object is automatically destroyed.
//!
//! @tparam T defines the derived class.
//! @tparam AllocationPolicy defies destroy policy.
//!
//! When reference counter becomes zero, AllocationPolicy::destroy() is invoked
//! by RefCounted to destroy itself.
//!
//! Inherits AllocationPolicy to make its methods and state available in the
//! derived class. E.g., PoolAllocation policy holds a reference to the pool
//! and uses it to release the object.
//!
//! Thread-safe.
template <class T, class AllocationPolicy>
class RefCounted : public NonCopyable<RefCounted<T, AllocationPolicy> >,
                   protected AllocationPolicy {
public:
    //! Initialize.
    RefCounted()
        : AllocationPolicy()
        , counter_(0) {
    }

    //! Initialize.
    explicit RefCounted(const AllocationPolicy& policy)
        : AllocationPolicy(policy)
        , counter_(0) {
    }

    ~RefCounted() {
        if (!counter_.compare_exchange(0, -1)) {
            roc_panic(
                "ref counter:"
                " attempt to destroy object that is in use, destroyed, or corrupted:"
                " counter=%d",
                (int)counter_);
        }
    }

    //! Get reference counter.
    int getref() const {
        const int current_counter = counter_;

        if (current_counter < 0 || current_counter > MaxCounter) {
            roc_panic("ref counter:"
                      " attempt to access destroyed or currupted object:"
                      " counter=%d",
                      (int)current_counter);
        }

        return current_counter;
    }

    //! Increment reference counter.
    void incref() const {
        const int previous_counter = counter_++;

        if (previous_counter < 0 || previous_counter > MaxCounter) {
            roc_panic("ref counter:"
                      " attempt to access destroyed or currupted object"
                      " counter=%d",
                      (int)previous_counter);
        }
    }

    //! Decrement reference counter.
    //! @remarks
    //!  Destroys itself if reference counter becomes zero.
    void decref() const {
        const int previous_counter = counter_--;

        if (previous_counter < 0 || previous_counter > MaxCounter) {
            roc_panic("ref counter:"
                      " attempt to access destroyed or currupted object"
                      " counter=%d",
                      (int)previous_counter);
        }

        if (previous_counter == 0) {
            roc_panic("ref counter: unpaired incref/decref");
        }

        if (previous_counter == 1) {
            const_cast<RefCounted&>(*this).destroy(
                static_cast<T&>(const_cast<RefCounted&>(*this)));
        }
    }

private:
    enum { MaxCounter = 100000 };

    mutable Atomic<int> counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REF_COUNTED_H_
