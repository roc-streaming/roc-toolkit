/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ref_counted.h
//! @brief Base class for reference counted object.

#ifndef ROC_CORE_REF_COUNTED_H_
#define ROC_CORE_REF_COUNTED_H_

#include "roc_core/allocation_policy.h"
#include "roc_core/atomic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Base class for reference counted object.
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
//! Inherits AllocationPolicy to make its methods available in the derived class.
//!
//! Thread-safe.
template <class T, class AllocationPolicy>
class RefCounted : public NonCopyable<RefCounted<T, AllocationPolicy> >,
                   protected AllocationPolicy {
public:
    //! Initialization with default allocation policy.
    RefCounted()
        : AllocationPolicy()
        , counter_(0) {
    }

    //! Initialization with arbitrary allocation policy.
    explicit RefCounted(const AllocationPolicy& policy)
        : AllocationPolicy(policy)
        , counter_(0) {
    }

    ~RefCounted() {
        if (!counter_.compare_exchange(0, -1)) {
            roc_panic("ref counter: attempt to destroy object that is still in use: "
                      "counter=%d",
                      (int)counter_);
        }
    }

    //! Get reference counter.
    long getref() const {
        return counter_;
    }

    //! Increment reference counter.
    void incref() const {
        const int previous_counter = counter_++;

        if (previous_counter < 0) {
            roc_panic("ref counter: attempt to call acquire on destroyed object");
        }
    }

    //! Decrement reference counter.
    //! @remarks
    //!  Destroys itself if reference counter becomes zero.
    void decref() const {
        const int previous_counter = counter_--;

        if (previous_counter < 0) {
            roc_panic("ref counter: attempt to call release on destroyed object");
        }

        if (previous_counter == 0) {
            roc_panic("ref counter: attempt to call release without acquire");
        }

        if (previous_counter == 1) {
            const_cast<RefCounted&>(*this).destroy(
                static_cast<T&>(const_cast<RefCounted&>(*this)));
        }
    }

private:
    mutable Atomic<int> counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REF_COUNTED_H_
