/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_gnu/roc_core/atomic_ops.h
//! @brief Atomic operations.

#ifndef ROC_CORE_ATOMIC_OPS_H_
#define ROC_CORE_ATOMIC_OPS_H_

namespace roc {
namespace core {

//! Atomic operations.
//! Implementation for GNU toolchains.
//! Unlike C++11 and C11 atomics, allows to work with raw memory.
class AtomicOps {
public:
    //! @name Memory fence
    //! @{

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

    //! @}

    //! @name Load
    //! @{

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

    //! @}

    //! @name Store
    //! @{

    //! Atomic store (no barrier).
    template <class T1, class T2 = T1> static inline void store_relaxed(T1& var, T2 val) {
        __atomic_store_n(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic store (release barrier).
    template <class T1, class T2 = T1> static inline void store_release(T1& var, T2 val) {
        __atomic_store_n(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic store (full barrier).
    template <class T1, class T2 = T1> static inline void store_seq_cst(T1& var, T2 val) {
        __atomic_store_n(&var, val, __ATOMIC_SEQ_CST);
    }

    //! @}

    //! @name Exchange
    //! @{

    //! Atomic exchange (no barrier).
    template <class T1, class T2 = T1>
    static inline T1 exchange_relaxed(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic exchange (acquire barrier).
    template <class T1, class T2 = T1>
    static inline T1 exchange_acquire(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic exchange (release barrier).
    template <class T1, class T2 = T1>
    static inline T1 exchange_release(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic exchange (acquire-release barrier).
    template <class T1, class T2 = T1>
    static inline T1 exchange_acq_rel(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_ACQ_REL);
    }

    //! Atomic exchange (full barrier).
    template <class T1, class T2 = T1>
    static inline T1 exchange_seq_cst(T1& var, T2 val) {
        return __atomic_exchange_n(&var, val, __ATOMIC_SEQ_CST);
    }

    //! @}

    //! @name CAS
    //! @{

    //! Atomic compare-and-swap (no barriers for success and failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_relaxed(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_RELAXED,
                                           __ATOMIC_RELAXED);
    }

    //! Atomic compare-and-swap (acquire barriers for success and failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_acquire(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_ACQUIRE,
                                           __ATOMIC_ACQUIRE);
    }

    //! Atomic compare-and-swap (acquire barrier for success, no barrier for failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_acquire_relaxed(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_ACQUIRE,
                                           __ATOMIC_RELAXED);
    }

    //! Atomic compare-and-swap (release barrier for success, no barrier for failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_release(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_RELEASE,
                                           __ATOMIC_RELAXED);
    }

    //! Atomic compare-and-swap (release barrier for success, no barrier for failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_release_relaxed(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_RELEASE,
                                           __ATOMIC_RELAXED);
    }

    //! Atomic compare-and-swap (acq_rel barrier for success, acquire for failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_acq_rel(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_ACQ_REL,
                                           __ATOMIC_ACQUIRE);
    }

    //! Atomic compare-and-swap (acq_rel barrier for success, no barrier for failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_acq_rel_relaxed(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_ACQ_REL,
                                           __ATOMIC_RELAXED);
    }

    //! Atomic compare-and-swap (seq_cst barriers for success and failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_seq_cst(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_SEQ_CST,
                                           __ATOMIC_SEQ_CST);
    }

    //! Atomic compare-and-swap (seq_cst barrier for success, no barrier for failure).
    template <class T1, class T2 = T1>
    static inline bool compare_exchange_seq_cst_relaxed(T1& var, T1& exp, T2 des) {
        return __atomic_compare_exchange_n(&var, &exp, des, 0, __ATOMIC_SEQ_CST,
                                           __ATOMIC_RELAXED);
    }

    //! @}

    //! @name Addition
    //! @{

    //! Atomic fetch-add (no barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_add_relaxed(T1& var, T2 val) {
        return __atomic_fetch_add(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic add-and-fetch (acquire barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_add_acquire(T1& var, T2 val) {
        return __atomic_fetch_add(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic add-and-fetch (release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_add_release(T1& var, T2 val) {
        return __atomic_fetch_add(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic add-and-fetch (acquire-release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_add_acq_rel(T1& var, T2 val) {
        return __atomic_fetch_add(&var, val, __ATOMIC_ACQ_REL);
    }

    //! Atomic add-and-fetch (full barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_add_seq_cst(T1& var, T2 val) {
        return __atomic_fetch_add(&var, val, __ATOMIC_SEQ_CST);
    }

    //! @}

    //! @name Subtraction
    //! @{

    //! Atomic fetch-sub (no barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_sub_relaxed(T1& var, T2 val) {
        return __atomic_fetch_sub(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic sub-and-fetch (acquire barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_sub_acquire(T1& var, T2 val) {
        return __atomic_fetch_sub(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic sub-and-fetch (release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_sub_release(T1& var, T2 val) {
        return __atomic_fetch_sub(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic sub-and-fetch (acquire-release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_sub_acq_rel(T1& var, T2 val) {
        return __atomic_fetch_sub(&var, val, __ATOMIC_ACQ_REL);
    }

    //! Atomic sub-and-fetch (full barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_sub_seq_cst(T1& var, T2 val) {
        return __atomic_fetch_sub(&var, val, __ATOMIC_SEQ_CST);
    }

    //! @}

    //! @name Bitwise AND
    //! @{

    //! Atomic fetch-and (no barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_and_relaxed(T1& var, T2 val) {
        return __atomic_fetch_and(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic fetch-and (acquire barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_and_acquire(T1& var, T2 val) {
        return __atomic_fetch_and(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic fetch-and (release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_and_release(T1& var, T2 val) {
        return __atomic_fetch_and(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic fetch-and (acquire-release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_and_acq_rel(T1& var, T2 val) {
        return __atomic_fetch_and(&var, val, __ATOMIC_ACQ_REL);
    }

    //! Atomic fetch-and (full barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_and_seq_cst(T1& var, T2 val) {
        return __atomic_fetch_and(&var, val, __ATOMIC_SEQ_CST);
    }

    //! @}

    //! @name Bitwise OR
    //! @{

    //! Atomic fetch-or (no barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_or_relaxed(T1& var, T2 val) {
        return __atomic_fetch_or(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic fetch-or (acquire barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_or_acquire(T1& var, T2 val) {
        return __atomic_fetch_or(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic fetch-or (release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_or_release(T1& var, T2 val) {
        return __atomic_fetch_or(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic fetch-or (acquire-release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_or_acq_rel(T1& var, T2 val) {
        return __atomic_fetch_or(&var, val, __ATOMIC_ACQ_REL);
    }

    //! Atomic fetch-or (full barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_or_seq_cst(T1& var, T2 val) {
        return __atomic_fetch_or(&var, val, __ATOMIC_SEQ_CST);
    }

    //! @}

    //! @name Bitwise XOR
    //! @{

    //! Atomic fetch-xor (no barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_xor_relaxed(T1& var, T2 val) {
        return __atomic_fetch_xor(&var, val, __ATOMIC_RELAXED);
    }

    //! Atomic fetch-xor (acquire barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_xor_acquire(T1& var, T2 val) {
        return __atomic_fetch_xor(&var, val, __ATOMIC_ACQUIRE);
    }

    //! Atomic fetch-xor (release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_xor_release(T1& var, T2 val) {
        return __atomic_fetch_xor(&var, val, __ATOMIC_RELEASE);
    }

    //! Atomic fetch-xor (acquire-release barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_xor_acq_rel(T1& var, T2 val) {
        return __atomic_fetch_xor(&var, val, __ATOMIC_ACQ_REL);
    }

    //! Atomic fetch-xor (full barrier).
    template <class T1, class T2 = T1>
    static inline T1 fetch_xor_seq_cst(T1& var, T2 val) {
        return __atomic_fetch_xor(&var, val, __ATOMIC_SEQ_CST);
    }

    //! @}
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ATOMIC_OPS_H_
