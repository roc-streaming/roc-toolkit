/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_libatomic_ops/roc_core/atomic.h
//! @brief Atomic integer.

#ifndef ROC_CORE_ATOMIC_H_
#define ROC_CORE_ATOMIC_H_

#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

#include <atomic_ops.h>

namespace roc {
namespace core {

//! Atomic integer.
class Atomic : public NonCopyable<> {
public:
    //! Initialize with given value.
    explicit Atomic(long v = 0) {
        AO_store(&value_, (AO_t)v);
    }

    //! Atomic load.
    operator long() const {
        return (long)AO_load(&value_);
    }

    //! Atomic store.
    long operator=(long v) {
        AO_store(&value_, (AO_t)v);
        return v;
    }

    //! Atomic increment.
    long operator++() {
        return long(AO_fetch_and_add1(&value_) + 1);
    }

    //! Atomic decrement.
    long operator--() {
        return long(AO_fetch_and_sub1(&value_) - 1);
    }

    //! Atomic add.
    long operator+=(long increment) {
        return long(AO_fetch_and_add(&value_, (AO_t)increment) + (AO_t)increment);
    }

    //! Atomic sub.
    long operator-=(long decrement) {
        return long(AO_fetch_and_add(&value_, (AO_t)-decrement) - (AO_t)decrement);
    }

private:
    mutable volatile AO_t value_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_H_
