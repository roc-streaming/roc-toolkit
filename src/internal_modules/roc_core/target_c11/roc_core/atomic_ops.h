/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_c11/roc_core/atomic_ops.h
//! @brief Atomic operations.

#ifndef ROC_CORE_ATOMIC_OPS_H_
#define ROC_CORE_ATOMIC_OPS_H_

namespace roc {
namespace core {

//! Atomic operations.
//! This wrapper exists because on non-C11 compilers we use another implementation.
class AtomicOps {
public:
    //! Acquire memory barrier.
    static inline void fence_acquire() {
        __atomic_thread_fence(__ATOMIC_ACQUIRE);
    }

    //! Release memory barrier.
    static inline void fence_release() {
        __atomic_thread_fence(__ATOMIC_RELEASE);
    }

    //! Full memory barrier.
    static inline void fence_seq_cst() {
        __atomic_thread_fence(__ATOMIC_SEQ_CST);
    }

    //! Atomic load (no barrier).
    template <class T> static inline T load_relaxed(const T& var) {
        return __atomic_load_n(&var, __ATOMIC_RELAXED);
    }

    //! Atomic load (acquire barrier).
    template <class T> static inline T load_acquire(const T& var) {
        return __atomic_load_n(&var, __ATOMIC_ACQUIRE);
    }

    //! Atomic load (full barrier).
    template <class T> static inline T load_seq_cst(const T& var) {
        return __atomic_load_n(&var, __ATOMIC_SEQ_CST);
    }

    //! Atomic store (no barrier).
    template <class T1, class T2> static inline void store_relaxed(T1& var, T2 val) {
        __atomic_store_n(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic store (release barrier).
    template <class T1, class T2> static inline void store_release(T1& var, T2 val) {
        __atomic_store_n(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic store (full barrier).
    template <class T1, class T2> static inline void store_seq_cst(T1& var, T2 val) {
        __atomic_store_n(&var, val, __ATOMIC_SEQ_CST);
    }

    //! Atomic exchange (no barrier).
    template <class T1, class T2> static inline T1 exchange_relaxed(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic exchange (acquire barrier).
    template <class T1, class T2> static inline T1 exchange_acquire(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic exchange (release barrier).
    template <class T1, class T2> static inline T1 exchange_release(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic exchange (acquire-release barrier).
    template <class T1, class T2> static inline T1 exchange_acq_rel(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_ACQ_REL);
    }

    //! Atomic exchange (full barrier).
    template <class T1, class T2> static inline T1 exchange_seq_cst(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_SEQ_CST);
    }

    //! Atomic compare-and-swap (no barrier).
    template <class T1, class T2, class T3>
    static inline bool compare_exchange_relaxed(T1& var, T2& exp, T3 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_RELAXED,
                                           __ATOMIC_RELAXED);
    }

    //! Atomic compare-and-swap (acquire barrier).
    template <class T1, class T2, class T3>
    static inline bool compare_exchange_acquire(T1& var, T2& exp, T3 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_ACQUIRE,
                                           __ATOMIC_ACQUIRE);
    }

    //! Atomic compare-and-swap (release barrier).
    template <class T1, class T2, class T3>
    static inline bool compare_exchange_release(T1& var, T2& exp, T3 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_RELEASE,
                                           __ATOMIC_RELAXED);
    }

    //! Atomic compare-and-swap (acquire-release barrier).
    template <class T1, class T2, class T3>
    static inline bool compare_exchange_acq_rel(T1& var, T2& exp, T3 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_ACQ_REL,
                                           __ATOMIC_ACQUIRE);
    }

    //! Atomic compare-and-swap (full barrier).
    template <class T1, class T2, class T3>
    static inline bool compare_exchange_seq_cst(T1& var, T2& exp, T3 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_SEQ_CST,
                                           __ATOMIC_SEQ_CST);
    }

    //! Atomic add-and-fetch (no barrier).
    template <class T1, class T2> static inline T1 add_fetch_relaxed(T1& var, T2 val) {
        return __atomic_add_fetch(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic add-and-fetch (acquire barrier).
    template <class T1, class T2> static inline T1 add_fetch_acquire(T1& var, T2 val) {
        return __atomic_add_fetch(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic add-and-fetch (release barrier).
    template <class T1, class T2> static inline T1 add_fetch_release(T1& var, T2 val) {
        return __atomic_add_fetch(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic add-and-fetch (full barrier).
    template <class T1, class T2> static inline T1 add_fetch_seq_cst(T1& var, T2 val) {
        return __atomic_add_fetch(&var, val, __ATOMIC_SEQ_CST);
    }

    //! Atomic sub-and-fetch (no barrier).
    template <class T1, class T2> static inline T1 sub_fetch_relaxed(T1& var, T2 val) {
        return __atomic_sub_fetch(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic sub-and-fetch (acquire barrier).
    template <class T1, class T2> static inline T1 sub_fetch_acquire(T1& var, T2 val) {
        return __atomic_sub_fetch(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic sub-and-fetch (release barrier).
    template <class T1, class T2> static inline T1 sub_fetch_release(T1& var, T2 val) {
        return __atomic_sub_fetch(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic sub-and-fetch (full barrier).
    template <class T1, class T2> static inline T1 sub_fetch_seq_cst(T1& var, T2 val) {
        return __atomic_sub_fetch(&var, val, __ATOMIC_SEQ_CST);
    }
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_OPS_H_
