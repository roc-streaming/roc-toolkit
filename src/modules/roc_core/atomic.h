/*
 * Copyright (c) 2020 Roc authors
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
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Atomic number.
//! Provides sequential consistency.
//! For a fine-grained memory order control, see AtomicOps.
template <class T> class Atomic : public NonCopyable<> {
public:
    //! Initialize with given value.
    explicit inline Atomic(T val)
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

    //! Atomic load.
    inline operator T() const {
        return AtomicOps::load_seq_cst(var_);
    }

    //! Atomic store.
    inline T operator=(T val) {
        AtomicOps::store_seq_cst(var_, val);
        return val;
    }

    //! Atomic increment.
    inline T operator++() {
        return AtomicOps::add_fetch_seq_cst(var_, 1);
    }

    //! Atomic decrement.
    inline T operator--() {
        return AtomicOps::sub_fetch_seq_cst(var_, 1);
    }

    //! Atomic addition.
    inline T operator+=(T val) {
        return AtomicOps::add_fetch_seq_cst(var_, val);
    }

    //! Atomic subtraction.
    inline T operator-=(T val) {
        return AtomicOps::sub_fetch_seq_cst(var_, val);
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
    explicit inline Atomic(T* val)
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

    //! Atomic increment.
    inline T* operator++() {
        return AtomicOps::add_fetch_seq_cst(var_, (ptrdiff_t)sizeof(T));
    }

    //! Atomic decrement.
    inline T* operator--() {
        return AtomicOps::sub_fetch_seq_cst(var_, (ptrdiff_t)sizeof(T));
    }

    //! Atomic addition.
    inline T* operator+=(ptrdiff_t val) {
        return AtomicOps::add_fetch_seq_cst(var_, val * (ptrdiff_t)sizeof(T));
    }

    //! Atomic subtraction.
    inline T* operator-=(ptrdiff_t val) {
        return AtomicOps::sub_fetch_seq_cst(var_, val * (ptrdiff_t)sizeof(T));
    }

private:
    T* var_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_H_
