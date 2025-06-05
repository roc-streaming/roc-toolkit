/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/atomic_bool.h
//! @brief Atomic bool.

#ifndef ROC_CORE_ATOMIC_BOOL_H_
#define ROC_CORE_ATOMIC_BOOL_H_

#include "roc_core/atomic_ops.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Portable atomic boolean.
//!
//! Provides sequential consistency (SEQ_CST).
//! For a fine-grained memory order control, see AtomicOps.
//!
//! See also notes for AtomicInt.
class AtomicBool : public NonCopyable<> {
public:
    //! Initialize with given value.
    explicit inline AtomicBool(bool val = false)
        : var_((uint32_t)val) {
    }

    //! Atomic exchange.
    inline bool exchange(bool val) {
        return (bool)AtomicOps::exchange_seq_cst(var_, (uint32_t)val);
    }

    //! Atomic compare-and-swap.
    inline bool compare_exchange(bool exp, bool des) {
        uint32_t exp_tmp = (uint32_t)exp;
        return AtomicOps::compare_exchange_seq_cst(var_, exp_tmp, (uint32_t)des);
    }

    //! Atomic load.
    inline operator bool() const {
        return (bool)AtomicOps::load_seq_cst(var_);
    }

    //! Atomic store.
    inline bool operator=(bool val) {
        AtomicOps::store_seq_cst(var_, (uint32_t)val);
        return val;
    }

private:
    uint32_t var_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_BOOL_H_
