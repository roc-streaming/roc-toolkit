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
#include "roc_core/noncopyable.h"
#include "roc_core/ref_counted_impl.h"

namespace roc {
namespace core {

//! Base class for object with reference counter.
//!
//! Allows to increment and decrement reference counter. When the counter
//! reaches zero, the object is automatically destroyed.
//!
//! @tparam T defines the derived class.
//! @tparam AllocationPolicy defines destroy policy.
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
        : AllocationPolicy() {
    }

    //! Initialize.
    explicit RefCounted(const AllocationPolicy& alloc)
        : AllocationPolicy(alloc) {
    }

    //! Get reference counter.
    int getref() const {
        return impl_.getref();
    }

    //! Increment reference counter.
    void incref() const {
        impl_.incref();
    }

    //! Decrement reference counter.
    //! @remarks
    //!  Destroys itself if reference counter becomes zero.
    void decref() const {
        const int current_counter = impl_.decref();

        if (current_counter == 0) {
            const_cast<RefCounted&>(*this).destroy(
                static_cast<T&>(const_cast<RefCounted&>(*this)));
        }
    }

private:
    RefCountedImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REF_COUNTED_H_
