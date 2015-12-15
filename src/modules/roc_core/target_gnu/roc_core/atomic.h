/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_gnu/roc_core/atomic.h
//! @brief Atomic integer.

#ifndef ROC_CORE_ATOMIC_H_
#define ROC_CORE_ATOMIC_H_

namespace roc {
namespace core {

//! Atomic integer.
class Atomic : public NonCopyable<> {
public:
    //! Initialize with given value.
    explicit Atomic(long value = 0)
        : value_(value) {
    }

    //! Atomic load.
    operator long() const {
        return __atomic_load_n(&value_, __ATOMIC_SEQ_CST);
    }

    //! Atomic store.
    long operator=(long v) {
        __atomic_store_n(&value_, v, __ATOMIC_SEQ_CST);
        return v;
    }

    //! Atomic increment.
    long operator++() {
        return __atomic_add_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }

    //! Atomic decrement.
    long operator--() {
        return __atomic_sub_fetch(&value_, 1, __ATOMIC_SEQ_CST);
    }

    //! Atomic test-and-set.
    //! @remarks
    //!  Atomically sets value to non-zero and returns '0' if previous value
    //!  was '0' or '1' otherwise.
    long test_and_set() {
        long expected = 0;
        return __atomic_compare_exchange_n(&value_, &expected, 1, false, __ATOMIC_SEQ_CST,
                                           __ATOMIC_SEQ_CST)
            ? 0
            : 1;
    }

private:
    mutable long value_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_H_
