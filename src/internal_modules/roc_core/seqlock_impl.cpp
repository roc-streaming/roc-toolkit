/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/seqlock_impl.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/cpu_instructions.h"

namespace {
// We use hand-rolled loop instead of memcpy() or default (trivial) copy constructor
// to be sure that the copying will be covered by our memory fences. On some
// platforms, memcpy() and copy constructor may be implemented using streaming
// instructions which may ignore memory fences.
void volatile_copy(volatile void* dst, const volatile void* src, size_t val_size) {
    volatile char* dst_ptr = reinterpret_cast<volatile char*>(dst);
    const volatile char* src_ptr = reinterpret_cast<const volatile char*>(src);

    for (size_t n = 0; n < val_size; n++) {
        dst_ptr[n] = src_ptr[n];
    }
}

} // namespace

namespace roc {
namespace core {

SeqlockImpl::SeqlockImpl()
    : ver_(0) {
}

seqlock_version_t SeqlockImpl::version() const {
    return AtomicOps::load_seq_cst(ver_);
}

bool SeqlockImpl::try_store(seqlock_version_t& ver,
                            void* current_value,
                            size_t value_size,
                            const void* new_value) {
    seqlock_version_t ver0 = AtomicOps::load_relaxed(ver_);
    if (ver0 & 1) {
        return false;
    }

    if (!AtomicOps::compare_exchange_relaxed(ver_, ver0, ver0 + 1)) {
        return false;
    }
    AtomicOps::fence_release();

    volatile_copy(current_value, new_value, value_size);
    AtomicOps::fence_seq_cst();

    ver = ver0 + 2;
    AtomicOps::store_relaxed(ver_, ver);

    return true;
}

void SeqlockImpl::exclusive_store(seqlock_version_t& ver,
                                  void* current_value,
                                  size_t value_size,
                                  const void* new_value) {
    const seqlock_version_t ver0 = AtomicOps::load_relaxed(ver_);
    AtomicOps::store_relaxed(ver_, ver0 + 1);
    AtomicOps::fence_release();

    volatile_copy(current_value, new_value, value_size);
    AtomicOps::fence_seq_cst();

    ver = ver0 + 2;
    AtomicOps::store_relaxed(ver_, ver);
}

// If the concurrent store is running and is not sleeping, retrying 3 times
// should be enough to succeed. This may fail if the concurrent store was
// preempted in the middle, of if there are multiple concurrent stores.
bool SeqlockImpl::try_load_repeat(seqlock_version_t& ver,
                                  const void* current_value,
                                  size_t value_size,
                                  void* return_value) const {
    if (try_load_(ver, current_value, value_size, return_value)) {
        return true;
    }
    if (try_load_(ver, current_value, value_size, return_value)) {
        return true;
    }
    if (try_load_(ver, current_value, value_size, return_value)) {
        return true;
    }
    return false;
}

void SeqlockImpl::wait_load(seqlock_version_t& ver,
                            const void* current_value,
                            size_t value_size,
                            void* return_value) const {
    while (!try_load_(ver, current_value, value_size, return_value)) {
        cpu_relax();
    }
}

bool SeqlockImpl::try_load_(seqlock_version_t& ver,
                            const void* current_value,
                            size_t value_size,
                            void* return_value) const {
    const seqlock_version_t ver0 = AtomicOps::load_relaxed(ver_);
    AtomicOps::fence_seq_cst();

    volatile_copy(return_value, current_value, value_size);
    AtomicOps::fence_acquire();

    ver = AtomicOps::load_relaxed(ver_);
    return ((ver0 & 1) == 0 && ver0 == ver);
}

} // namespace core
} // namespace roc
