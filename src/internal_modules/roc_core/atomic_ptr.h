/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/atomic_ptr.h
//! @brief Atomic pointer.

#ifndef ROC_CORE_ATOMIC_PTR_H_
#define ROC_CORE_ATOMIC_PTR_H_

#include "roc_core/atomic_ops.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Portable atomic pointer.
//!
//! Provides sequential consistency (SEQ_CST).
//! For a fine-grained memory order control, see AtomicOps.
//!
//! See also notes for AtomicInt.
template <class T> class AtomicPtr : public NonCopyable<> {
public:
    //! Initialize with given value.
    explicit inline AtomicPtr(T* val = nullptr)
        : var_(val) {
    }

    //! Atomic exchange.
    inline T* exchange(T* val) {
        return AtomicOps::exchange_seq_cst(var_, val);
    }

    //! Atomic compare-and-swap.
    inline bool compare_exchange(T* exp, T* des) {
        return AtomicOps::compare_exchange_seq_cst(var_, exp, des);
    }

    //! Atomic load.
    inline T* operator->() const {
        T* ptr = AtomicOps::load_seq_cst(var_);
        if (!ptr) {
            roc_panic("atomic ptr: attempt to dereference null pointer");
        }
        return ptr;
    }

    //! Atomic load.
    inline T& operator*() const {
        T* ptr = AtomicOps::load_seq_cst(var_);
        if (!ptr) {
            roc_panic("atomic ptr: attempt to dereference null pointer");
        }
        return *ptr;
    }

    //! Atomic load.
    inline operator T*() const {
        return AtomicOps::load_seq_cst(var_);
    }

    //! Atomic store.
    inline T* operator=(T* val) {
        AtomicOps::store_seq_cst(var_, val);
        return val;
    }

    //! Atomic increment (prefix).
    inline T* operator++() {
        return AtomicOps::fetch_add_seq_cst(var_, ptrdiff_t(sizeof(T))) + 1;
    }

    //! Atomic increment (postfix).
    inline T* operator++(int) {
        return AtomicOps::fetch_add_seq_cst(var_, ptrdiff_t(sizeof(T)));
    }

    //! Atomic decrement (prefix).
    inline T* operator--() {
        return AtomicOps::fetch_sub_seq_cst(var_, ptrdiff_t(sizeof(T))) - 1;
    }

    //! Atomic decrement (postfix).
    inline T* operator--(int) {
        return AtomicOps::fetch_sub_seq_cst(var_, ptrdiff_t(sizeof(T)));
    }

    //! Atomic addition.
    inline T* operator+=(ptrdiff_t val) {
        return AtomicOps::fetch_add_seq_cst(var_, val * ptrdiff_t(sizeof(T))) + val;
    }

    //! Atomic subtraction.
    inline T* operator-=(ptrdiff_t val) {
        return AtomicOps::fetch_sub_seq_cst(var_, val * ptrdiff_t(sizeof(T))) - val;
    }

private:
    T* var_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_PTR_H_
