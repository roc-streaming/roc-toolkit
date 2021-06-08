/*
 * Copyright (c) 2021 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/usage_counter.h
//! @brief Base class for object with usage counter.

#ifndef ROC_CORE_USAGE_COUNTER_H_
#define ROC_CORE_USAGE_COUNTER_H_

#include "roc_core/atomic.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Base class for object with usage counter.
//!
//! Allows to increment and descrement usage counter of the object. Checks the
//! counter in destructor and panics if it's non-zero.
//!
//! Thread-safe.
class UsageCounter : public NonCopyable<UsageCounter> {
public:
    UsageCounter()
        : counter_(0) {
    }

    ~UsageCounter() {
        if (!counter_.compare_exchange(0, -1)) {
            roc_panic("usage counter: attempt to destroy object that is still in use: "
                      "usage_counter=%d",
                      (int)counter_);
        }
    }

    //! Check whether usage counter is non-zero.
    bool is_used() const {
        const int current_counter = counter_;

        if (current_counter < 0) {
            roc_panic("usage counter: attempt to access destroyed object");
        }

        return current_counter > 0;
    }

    //! Increment usage counter.
    void acquire_usage() const {
        const int previous_counter = counter_++;

        if (previous_counter < 0) {
            roc_panic("usage counter: attempt to call acquire on destroyed object");
        }
    }

    //! Decrement usage counter.
    void release_usage() const {
        const int previous_counter = counter_--;

        if (previous_counter < 0) {
            roc_panic("usage counter: attempt to call release on destroyed object");
        }

        if (previous_counter == 0) {
            roc_panic("usage counter: attempt to call release without acquire");
        }
    }

private:
    mutable Atomic<int> counter_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_USAGE_COUNTER_H_
