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
//! reaches zero, the object is automatically disposed.
//!
//! @tparam T defines the derived class.
//! @tparam AllocationPolicy defines allocation policy to be inherited
//! (ArenaAllocation, PoolAllocation, etc.)
//!
//! When reference counter becomes zero, RefCounted invokes dispose() method
//! implemented by allocation policy.
//!
//! Thread-safe.
template <class T, class AllocationPolicy>
class RefCounted : public AllocationPolicy,
                   public NonCopyable<RefCounted<T, AllocationPolicy> > {
public:
    //! Initialize.
    RefCounted()
        : AllocationPolicy() {
    }

    //! Initialize.
    template <class Arg>
    explicit RefCounted(Arg& arg)
        : AllocationPolicy(arg) {
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
            const_cast<RefCounted&>(*this).dispose();
        }
    }

private:
    RefCountedImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REF_COUNTED_H_
