/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/atomic_size.h
//! @brief Atomic size_t.

#ifndef ROC_CORE_ATOMIC_SIZE_H_
#define ROC_CORE_ATOMIC_SIZE_H_

#include "roc_core/atomic_ops.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Portable atomic size_t (pointer-size integer).
//!
//! Provides sequential consistency (SEQ_CST).
//! For a fine-grained memory order control, see AtomicOps.
//!
//! See also notes for AtomicInt.
class AtomicSize : public NonCopyable<> {
public:
    //! Initialize with given value.
    explicit inline AtomicSize(size_t val = 0)
        : var_(val) {
    }

    //! Atomic exchange.
    inline size_t exchange(size_t val) {
        return AtomicOps::exchange_seq_cst(var_, val);
    }

    //! Atomic compare-and-swap.
    inline bool compare_exchange(size_t exp, size_t des) {
        return AtomicOps::compare_exchange_seq_cst(var_, exp, des);
    }

    //! Atomic load.
    inline operator size_t() const {
        return AtomicOps::load_seq_cst(var_);
    }

    //! Atomic store.
    inline size_t operator=(size_t val) {
        AtomicOps::store_seq_cst(var_, val);
        return val;
    }

    //! Atomic increment (prefix).
    inline size_t operator++() {
        return AtomicOps::fetch_add_seq_cst(var_, size_t(1)) + size_t(1);
    }

    //! Atomic increment (postfix).
    inline size_t operator++(int) {
        return AtomicOps::fetch_add_seq_cst(var_, size_t(1));
    }

    //! Atomic decrement (prefix).
    inline size_t operator--() {
        return AtomicOps::fetch_sub_seq_cst(var_, size_t(1)) - size_t(1);
    }

    //! Atomic decrement (postfix).
    inline size_t operator--(int) {
        return AtomicOps::fetch_sub_seq_cst(var_, size_t(1));
    }

    //! Atomic addition.
    inline size_t operator+=(size_t val) {
        return AtomicOps::fetch_add_seq_cst(var_, val) + val;
    }

    //! Atomic subtraction.
    inline size_t operator-=(size_t val) {
        return AtomicOps::fetch_sub_seq_cst(var_, val) - val;
    }

private:
    size_t var_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_SIZE_H_
