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

#include "roc_core/noncopyable.h"
#include "roc_core/seqlock_impl.h"
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
        : val_(value) {
    }

    //! Load value version.
    //! Wait-free.
    inline seqlock_version_t version() const {
        return impl_.version();
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
        return impl_.try_store(ver, &val_, sizeof(val_), &value);
    }

    //! Store value.
    //! Like try_store(), but also returns updated version.
    inline bool try_store_ver(const T& value, seqlock_version_t& ver) {
        return impl_.try_store(ver, &val_, sizeof(val_), &value);
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
        impl_.exclusive_store(ver, &val_, sizeof(val_), &value);
    }

    //! Store value.
    //! Like exclusive_store(), but also returns updated version.
    inline void exclusive_store_ver(const T& value, seqlock_version_t& ver) {
        impl_.exclusive_store(ver, &val_, sizeof(val_), &value);
    }

    //! Try to load value.
    //! Returns true if the value was loaded.
    //! May return false if concurrent store is currently in progress.
    //! Is both lock-free and wait-free, i.e. it never waits for sleeping threads
    //! and never spins.
    inline bool try_load(T& value) const {
        seqlock_version_t ver;
        return impl_.try_load_repeat(ver, &val_, sizeof(val_), &value);
    }

    //! Try to load value and version.
    //! Like try_load(), but also returns version.
    inline bool try_load_ver(T& value, seqlock_version_t& ver) const {
        return impl_.try_load_repeat(ver, &val_, sizeof(val_), &value);
    }

    //! Load value.
    //! May spin until concurrent store completes.
    //! Is NOT lock-free (or wait-free).
    inline T wait_load() const {
        T value;
        seqlock_version_t ver;
        impl_.wait_load(ver, &val_, sizeof(val_), &value);
        return value;
    }

    //! Load value and version.
    //! Like wait_load(), but also returns version.
    inline void wait_load_ver(T& value, seqlock_version_t& ver) const {
        impl_.wait_load(ver, &val_, sizeof(val_), &value);
    }

private:
    T val_;
    SeqlockImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SEQLOCK_H_
