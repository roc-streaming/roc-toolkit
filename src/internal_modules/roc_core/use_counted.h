/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/use_counted.h
//! @brief Base class for object with usage counter.

#ifndef ROC_CORE_USE_COUNTED_H_
#define ROC_CORE_USE_COUNTED_H_

#include "roc_core/atomic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Base class for object with usage counter.
//!
//! Allows to increment and descrement use counter of the object. Checks the
//! counter in destructor and panics if it's non-zero.
//!
//! Thread-safe.
class UseCounted : public NonCopyable<UseCounted> {
public:
    UseCounted()
        : counter_(0) {
    }

    ~UseCounted() {
        if (!counter_.compare_exchange(0, -1)) {
            roc_panic(
                "use counter:"
                " attempt to destroy object that is in use, destroyed, or corrupted:"
                " counter=%d",
                (int)counter_);
        }
    }

    //! Get reference counter.
    int getref() const {
        const int current_counter = counter_;

        if (current_counter < 0 || current_counter > MaxCounter) {
            roc_panic("use counter:"
                      " attempt to access destroyed or currupted object:"
                      " counter=%d",
                      (int)current_counter);
        }

        return current_counter;
    }

    //! Increment use counter.
    void incref() const {
        const int previous_counter = counter_++;

        if (previous_counter < 0 || previous_counter > MaxCounter) {
            roc_panic("use counter:"
                      " attempt to access destroyed or currupted object"
                      " counter=%d",
                      (int)previous_counter);
        }
    }

    //! Decrement use counter.
    //! @remarks
    //!  There is no special action when the counter becomes zero.
    void decref() const {
        const int previous_counter = counter_--;

        if (previous_counter < 0 || previous_counter > MaxCounter) {
            roc_panic("use counter:"
                      " attempt to access destroyed or currupted object"
                      " counter=%d",
                      (int)previous_counter);
        }

        if (previous_counter == 0) {
            roc_panic("use counter: unpaired incref/decref");
        }
    }

private:
    enum { MaxCounter = 100000 };

    mutable Atomic<int> counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_USE_COUNTED_H_
