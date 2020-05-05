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

//! Atomic base class.
template <class T> class AtomicBase : public NonCopyable<> {
public:
    //! Atomic load (no barrier).
    inline T load_relaxed() const {
        return AtomicOps::load_relaxed(var_);
    }

    //! Atomic load (acquire barrier).
    inline T load_acquire() const {
        return AtomicOps::load_acquire(var_);
    }

    //! Atomic store (no barrier).
    inline void store_relaxed(T val) {
        AtomicOps::store_relaxed(var_, val);
    }

    //! Atomic store (release barrrier).
    inline void store_release(T val) {
        AtomicOps::store_release(var_, val);
    }

    //! Atomic exchange (acquire barrier).
    inline T exchange_acquire(T val) {
        return AtomicOps::exchange_acquire(var_, val);
    }

    //! Atomic exchange (release barrier).
    inline T exchange_release(T val) {
        return AtomicOps::exchange_release(var_, val);
    }

    //! Atomic exchange (acquire-release barrier).
    inline T exchange_acq_rel(T val) {
        return AtomicOps::exchange_acq_rel(var_, val);
    }

    //! Atomic exchange (acquire barrier).
    inline bool compare_exchange_acquire(T exp, T des) {
        return AtomicOps::compare_exchange_acquire(var_, exp, des);
    }

    //! Atomic exchange (release barrier).
    inline bool compare_exchange_release(T exp, T des) {
        return AtomicOps::compare_exchange_release(var_, exp, des);
    }

    //! Atomic exchange (acquire-release barrier).
    inline bool compare_exchange_acq_rel(T exp, T des) {
        return AtomicOps::compare_exchange_acq_rel(var_, exp, des);
    }

protected:
    //! Initialize with given value.
    inline AtomicBase(T val)
        : var_(val) {
    }

    //! Underlying value.
    T var_;
};

//! Atomic number.
template <class T> class Atomic : public AtomicBase<T> {
public:
    //! Initialize with given value.
    explicit inline Atomic(T val = T())
        : AtomicBase<T>(val) {
    }

    //! Atomic load.
    inline operator T() const {
        return AtomicOps::load_seq_cst(this->var_);
    }

    //! Atomic store.
    inline T operator=(T val) {
        AtomicOps::store_seq_cst(this->var_, val);
        return val;
    }

    //! Atomic increment.
    inline T operator++() {
        return AtomicOps::add_fetch_seq_cst(this->var_, 1);
    }

    //! Atomic decrement.
    inline T operator--() {
        return AtomicOps::sub_fetch_seq_cst(this->var_, 1);
    }

    //! Atomic addition.
    inline T operator+=(T val) {
        return AtomicOps::add_fetch_seq_cst(this->var_, val);
    }

    //! Atomic subtraction.
    inline T operator-=(T val) {
        return AtomicOps::sub_fetch_seq_cst(this->var_, val);
    }
};

//! Atomic pointer.
template <class T> class Atomic<T*> : public AtomicBase<T*> {
public:
    //! Initialize with given value.
    explicit inline Atomic(T* val = NULL)
        : AtomicBase<T*>(val) {
    }

    //! Atomic load.
    T* operator->() const {
        return AtomicOps::load_seq_cst(this->var_);
    }

    //! Atomic load.
    T& operator*() const {
        return *AtomicOps::load_seq_cst(this->var_);
    }

    //! Atomic load.
    inline operator T*() const {
        return AtomicOps::load_seq_cst(this->var_);
    }

    //! Atomic store.
    inline T* operator=(T* val) {
        AtomicOps::store_seq_cst(this->var_, val);
        return val;
    }

    //! Atomic increment.
    inline T* operator++() {
        return AtomicOps::add_fetch_seq_cst(this->var_, (ptrdiff_t)sizeof(T));
    }

    //! Atomic decrement.
    inline T* operator--() {
        return AtomicOps::sub_fetch_seq_cst(this->var_, (ptrdiff_t)sizeof(T));
    }

    //! Atomic addition.
    inline T* operator+=(ptrdiff_t val) {
        return AtomicOps::add_fetch_seq_cst(this->var_, val * (ptrdiff_t)sizeof(T));
    }

    //! Atomic subtraction.
    inline T* operator-=(ptrdiff_t val) {
        return AtomicOps::sub_fetch_seq_cst(this->var_, val * (ptrdiff_t)sizeof(T));
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_H_
