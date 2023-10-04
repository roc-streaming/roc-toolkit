/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/mpsc_queue.h
//! @brief Multi-producer single-consumer queue.

#ifndef ROC_CORE_MPSC_QUEUE_H_
#define ROC_CORE_MPSC_QUEUE_H_

#include "roc_core/atomic_ops.h"
#include "roc_core/mpsc_queue_impl.h"
#include "roc_core/mpsc_queue_node.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Thread-safe lock-free node-based intrusive multi-producer single-consumer queue.
//!
//! Provides sequential consistency.
//!
//! Based on Dmitry Vyukov algorithm:
//!  - http://tiny.cc/3d3moz
//!  - https://int08h.com/post/ode-to-a-vyukov-queue/
//!  - https://github.com/samanbarghi/MPSCQ
//!
//! @tparam T defines object type, it should inherit MpscQueueNode.
//!
//! @tparam OwnershipPolicy defines ownership policy which is used to acquire an
//! element ownership when it's added to the queue and release ownership when it's
//! removed from the queue.
template <class T, template <class TT> class OwnershipPolicy = RefCountedOwnership>
class MpscQueue : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename OwnershipPolicy<T>::Pointer Pointer;

    ~MpscQueue() {
        // release ownership of all objects
        while (pop_front_exclusive()) {
        }
    }

    //! Add object to the end of the queue.
    //! Can be called concurrently.
    //! Acquires ownership of @p obj.
    //! After this call returns, any thread calling pop_front_exclusive() or
    //! try_pop_front_exclusive() is guaranteed to see a non-empty queue. But note
    //! that the latter can still fail if called concurrently with push_back().
    //! @note
    //!  - On CPUs with atomic exchange, e.g. x86, this operation is both lock-free
    //!    and wait-free, i.e. it never waits for sleeping threads and never spins.
    //!  - On CPUs without atomic exchange, e.g. arm64, this operation is lock-free,
    //!    but not wait-free, i.e. it never waits for sleeping threads, but with a low
    //!    probability can spin while there are concurrent non-sleeping push_back()
    //!    calls (because of the spin loop in the implementation of atomic exchange).
    //!  - Concurrent try_pop_front() and pop_front() does not affect this operation.
    //!    Only concurrent push_back() calls can make it spin.
    void push_back(T& obj) {
        OwnershipPolicy<T>::acquire(obj);

        MpscQueueNode::MpscQueueData* node = obj.mpsc_queue_data();

        change_owner_(node, NULL, this);

        impl_.push_back(node);
    }

    //! Try to remove object from the beginning of the queue (non-blocking version).
    //! Should NOT be called concurrently.
    //! Releases ownership of the returned object.
    //! @remarks
    //!  - Returns NULL if the queue is empty.
    //!  - May return NULL even if the queue is actially non-empty, in particular if
    //!    concurrent push_back() call is running, or if the push_back() results were
    //!    not fully published yet.
    //! @note
    //!  - This operation is both lock-free and wait-free on all architectures, i.e. it
    //!    never waits for sleeping threads and never spins indefinitely.
    Pointer try_pop_front_exclusive() {
        MpscQueueNode::MpscQueueData* node = impl_.pop_front<false>();
        if (!node) {
            return NULL;
        }

        change_owner_(node, this, NULL);

        Pointer obj = static_cast<T*>(node->container_of());
        OwnershipPolicy<T>::release(*obj);

        return obj;
    }

    //! Remove object from the beginning of the queue (blocking version).
    //! Should NOT be called concurrently.
    //! Releases ownership of the returned object.
    //! @remarks
    //!  - Returns NULL if the queue is empty.
    //!  - May spin while a concurrent push_back() call is running.
    //! @remarks
    //!  - This operation is NOT lock-free (or wait-free). It may spin until all
    //!    concurrent push_back() calls are finished.
    //!  - On the "fast-path", however, this operation does not wait for any
    //!    threads and just performs a few atomic reads and writes.
    Pointer pop_front_exclusive() {
        MpscQueueNode::MpscQueueData* node = impl_.pop_front<true>();
        if (!node) {
            return NULL;
        }

        change_owner_(node, this, NULL);

        Pointer obj = static_cast<T*>(node->container_of());
        OwnershipPolicy<T>::release(*obj);

        return obj;
    }

private:
    typedef MpscQueueNode::MpscQueueData MpscQueueData;

    void change_owner_(MpscQueueData* node, void* from, void* to) {
        void* exp = from;
        if (!AtomicOps::compare_exchange_relaxed(node->queue, exp, to)) {
            roc_panic("mpsc queue: unexpected node owner: from=%p to=%p cur=%p", from, to,
                      exp);
        }
    }

    MpscQueueImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MPSC_QUEUE_H_
