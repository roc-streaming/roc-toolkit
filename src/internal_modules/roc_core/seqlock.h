/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/seqlock.h
//! @brief Seqlock.

#ifndef ROC_CORE_SEQLOCK_H_
#define ROC_CORE_SEQLOCK_H_

#include "roc_core/atomic_ops.h"
#include "roc_core/cpu_instructions.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Type for holding seqlock value version.
//! Version is changed each value update.
//! May wrap.
typedef uint32_t seqlock_version_t;

//! Check if given seqlock version corresponds to dirty value.
inline bool seqlock_version_is_dirty(seqlock_version_t ver) {
    return (ver & 1) != 0;
}

//! Seqlock.
//!
//! Provides safe concurrent access to a single value.
//! Provides sequential consistency.
//! Optimized for infrequent writes and frequent reads.
//! Writes are lock-free and take priority over reads.
//!
//! See details on the barriers here:
//!  https://elixir.bootlin.com/linux/latest/source/include/linux/seqlock.h
//!  https://www.hpl.hp.com/techreports/2012/HPL-2012-68.pdf
template <class T> class Seqlock : public NonCopyable<> {
public:
    //! Initialize with given value.
    explicit Seqlock(T value)
        : val_(value)
        , ver_(0) {
    }

    //! Load value version.
    //! Wait-free.
    inline seqlock_version_t version() const {
        return load_version_();
    }

    //! Store value.
    //! Can be called concurrently, but only one concurrent call will succeed.
    //! Is both lock-free and wait-free, i.e. it never waits for sleeping threads
    //! and never spins.
    //! After this call returns, any thread calling wait_load() is guaranteed to
    //! get the updated value, and try_load() is guaranteed either return the
    //! updated value or fail (if changes are not fully published yet).
    inline bool try_store(const T& value) {
        seqlock_version_t ver;
        return try_store_(value, ver);
    }

    //! Store value.
    //! Like try_store(), but also returns updated version.
    inline bool try_store_ver(const T& value, seqlock_version_t& ver) {
        return try_store_(value, ver);
    }

    //! Store value.
    //! Can NOT be called concurrently, assumes that writes are serialized.
    //! Is both lock-free and wait-free, i.e. it never waits for sleeping threads
    //! and never spins.
    //! After this call returns, any thread calling wait_load() is guaranteed to
    //! get the updated value, and try_load() is guaranteed either return the
    //! updated value or fail (if changes are not fully published yet).
    inline void exclusive_store(const T& value) {
        seqlock_version_t ver;
        exclusive_store_(value, ver);
    }

    //! Store value.
    //! Like exclusive_store(), but also returns updated version.
    inline void exclusive_store_ver(const T& value, seqlock_version_t& ver) {
        exclusive_store_(value, ver);
    }

    //! Try to load value.
    //! Returns true if the value was loaded.
    //! May return false if concurrent store is currently in progress.
    //! Is both lock-free and wait-free, i.e. it never waits for sleeping threads
    //! and never spins.
    inline bool try_load(T& value) const {
        seqlock_version_t ver;
        return try_load_repeat_(value, ver);
    }

    //! Try to load value and version.
    //! Like try_load(), but also returns version.
    inline bool try_load_ver(T& value, seqlock_version_t& ver) const {
        return try_load_repeat_(value, ver);
    }

    //! Load value.
    //! May spin until concurrent store completes.
    //! Is NOT lock-free (or wait-free).
    inline T wait_load() const {
        T value;
        seqlock_version_t ver;
        wait_load_(value, ver);
        return value;
    }

    //! Load value and version.
    //! Like wait_load(), but also returns version.
    inline void wait_load_ver(T& value, seqlock_version_t& ver) const {
        wait_load_(value, ver);
    }

private:
    inline seqlock_version_t load_version_() const {
        return AtomicOps::load_seq_cst(ver_);
    }

    inline void exclusive_store_(const T& value, seqlock_version_t& ver) {
        const seqlock_version_t ver0 = AtomicOps::load_relaxed(ver_);
        AtomicOps::store_relaxed(ver_, ver0 + 1);
        AtomicOps::fence_release();

        volatile_copy_(val_, value);
        AtomicOps::fence_seq_cst();

        ver = ver0 + 2;
        AtomicOps::store_relaxed(ver_, ver);
    }

    inline bool try_store_(const T& value, seqlock_version_t& ver) {
        seqlock_version_t ver0 = AtomicOps::load_relaxed(ver_);
        if (ver0 & 1) {
            return false;
        }

        if (!AtomicOps::compare_exchange_relaxed(ver_, ver0, ver0 + 1)) {
            return false;
        }
        AtomicOps::fence_release();

        volatile_copy_(val_, value);
        AtomicOps::fence_seq_cst();

        ver = ver0 + 2;
        AtomicOps::store_relaxed(ver_, ver);

        return true;
    }

    inline void wait_load_(T& value, seqlock_version_t& ver) const {
        while (!try_load_(value, ver)) {
            cpu_relax();
        }
    }

    // If the concurrent store is running and is not sleeping, retrying 3 times
    // should be enough to succeed. This may fail if the concurrent store was
    // preempted in the middle, of if there are multiple concurrent stores.
    inline bool try_load_repeat_(T& value, seqlock_version_t& ver) const {
        if (try_load_(value, ver)) {
            return true;
        }
        if (try_load_(value, ver)) {
            return true;
        }
        if (try_load_(value, ver)) {
            return true;
        }
        return false;
    }

    inline bool try_load_(T& value, seqlock_version_t& ver) const {
        const seqlock_version_t ver0 = AtomicOps::load_relaxed(ver_);
        AtomicOps::fence_seq_cst();

        volatile_copy_(value, val_);
        AtomicOps::fence_acquire();

        ver = AtomicOps::load_relaxed(ver_);
        return ((ver0 & 1) == 0 && ver0 == ver);
    }

    // We use hand-rolled loop instead of memcpy() or default (trivial) copy constructor
    // to be sure that the copying will be covered by our memory fences. On some
    // platforms, memcpy() and copy constructor may be implemented using streaming
    // instructions which may ignore memory fences.
    static void volatile_copy_(volatile T& dst, const volatile T& src) {
        volatile char* dst_ptr = reinterpret_cast<volatile char*>(&dst);
        const volatile char* src_ptr = reinterpret_cast<const volatile char*>(&src);

        for (size_t n = 0; n < sizeof(T); n++) {
            dst_ptr[n] = src_ptr[n];
        }
    }

    T val_;
    seqlock_version_t ver_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SEQLOCK_H_
