/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/ref_counter.h
//! @brief Base class for reference countable object.

#ifndef ROC_CORE_REF_COUNTER_H_
#define ROC_CORE_REF_COUNTER_H_

#include "roc_core/atomic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Base class for reference countable object.
//!
//! Allows to increment and decrement reference counter. When the counter
//! reaches zero, the object is automatically destroyed using destroy()
//! method of the derived class.
//!
//! @tparam T defines the derived class.
//!
//! Thread-safe.
template <class T> class RefCounter : public NonCopyable<RefCounter<T> > {
public:
    RefCounter()
        : counter_(0) {
    }

    ~RefCounter() {
        if (!counter_.compare_exchange(0, -1)) {
            roc_panic("ref counter: attempt to destroy object that is still in use: "
                      "ref_counter=%d",
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
    //!  Calls destroy() if reference counter becomes zero.
    void decref() const {
        const int previous_counter = counter_--;

        if (previous_counter < 0) {
            roc_panic("ref counter: attempt to call release on destroyed object");
        }

        if (previous_counter == 0) {
            roc_panic("ref counter: attempt to call release without acquire");
        }

        if (previous_counter == 1) {
            static_cast<T*>(const_cast<RefCounter*>(this))->destroy();
        }
    }

private:
    mutable Atomic<int> counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_REF_COUNTER_H_
