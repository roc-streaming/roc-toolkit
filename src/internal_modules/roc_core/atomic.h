/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/atomic.h
//! @brief Atomic.

#ifndef ROC_CORE_ATOMIC_H_
#define ROC_CORE_ATOMIC_H_

#include "roc_core/atomic_ops.h"
#include "roc_core/cpu_traits.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Atomic integer.
//! Provides sequential consistency.
//! For a fine-grained memory order control, see AtomicOps.
template <class T> class Atomic : public NonCopyable<> {
    // Allow only types that are available natively on all architectures.
    struct TypeCheck {
        int f : sizeof(T) == 8 || sizeof(T) == 16 || sizeof(T) == 32 ? 1 : -1;
    };

public:
    //! Initialize with given value.
    explicit inline Atomic(T val = 0)
        : var_(val) {
    }

    //! Atomic exchange.
    inline T exchange(T val) {
        return AtomicOps::exchange_seq_cst(var_, val);
    }

    //! Atomic compare-and-swap.
    inline bool compare_exchange(T exp, T des) {
        return AtomicOps::compare_exchange_seq_cst(var_, exp, des);
    }

    //! Atomic fetch-or.
    inline T fetch_or(T val) {
        return AtomicOps::fetch_or_seq_cst(var_, val);
    }

    //! Atomic fetch-and.
    inline T fetch_and(T val) {
        return AtomicOps::fetch_and_seq_cst(var_, val);
    }

    //! Atomic fetch-xor.
    inline T fetch_xor(T val) {
        return AtomicOps::fetch_xor_seq_cst(var_, val);
    }

    //! Atomic load.
    inline operator T() const {
        return AtomicOps::load_seq_cst(var_);
    }

    //! Atomic store.
    inline T operator=(T val) {
        AtomicOps::store_seq_cst(var_, val);
        return val;
    }

    //! Atomic increment (prefix).
    inline T operator++() {
        return AtomicOps::fetch_add_seq_cst(var_, T(1)) + T(1);
    }

    //! Atomic increment (postfix).
    inline T operator++(int) {
        return AtomicOps::fetch_add_seq_cst(var_, T(1));
    }

    //! Atomic decrement (prefix).
    inline T operator--() {
        return AtomicOps::fetch_sub_seq_cst(var_, T(1)) - T(1);
    }

    //! Atomic decrement (postfix).
    inline T operator--(int) {
        return AtomicOps::fetch_sub_seq_cst(var_, T(1));
    }

    //! Atomic addition.
    inline T operator+=(T val) {
        return AtomicOps::fetch_add_seq_cst(var_, val) + val;
    }

    //! Atomic subtraction.
    inline T operator-=(T val) {
        return AtomicOps::fetch_sub_seq_cst(var_, val) - val;
    }

    //! Atomic bitwise or.
    inline T operator|=(T val) {
        return AtomicOps::fetch_or_seq_cst(var_, val) | val;
    }

    //! Atomic bitwise and.
    inline T operator&=(T val) {
        return AtomicOps::fetch_and_seq_cst(var_, val) & val;
    }

    //! Atomic bitwise xor.
    inline T operator^=(T val) {
        return AtomicOps::fetch_xor_seq_cst(var_, val) ^ val;
    }

private:
    T var_;
};

//! Atomic pointer.
//! Provides sequential consistency.
//! For a fine-grained memory order control, see AtomicOps.
template <class T> class Atomic<T*> : public NonCopyable<> {
public:
    //! Initialize with given value.
    explicit inline Atomic(T* val = NULL)
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
    T* operator->() const {
        return AtomicOps::load_seq_cst(var_);
    }

    //! Atomic load.
    T& operator*() const {
        return *AtomicOps::load_seq_cst(var_);
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

#endif // ROC_CORE_ATOMIC_H_
