/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/mpsc_queue.h
//! @brief Multi-producer single-consumer queue.

#ifndef ROC_CORE_MPSC_QUEUE_H_
#define ROC_CORE_MPSC_QUEUE_H_

#include "roc_core/atomic.h"
#include "roc_core/cpu_ops.h"
#include "roc_core/mpsc_queue_node.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! Thread-safe lock-free node-based intrusive multi-producer single-consumer queue.
//!
//! Based on Dmitry Vyukov algorithm:
//!  - http://tiny.cc/3d3moz
//!  - https://int08h.com/post/ode-to-a-vyukov-queue/
//!  - https://github.com/samanbarghi/MPSCQ
//!
//! @tparam T defines object type, it should inherit MpscQueueNode.
//!
//! @tparam Ownership defines ownership policy which is used to acquire an element
//! ownership when it's added to the queue and release ownership when it's removed
//! from the queue.
template <class T, template <class TT> class Ownership = RefCntOwnership>
class MpscQueue : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename Ownership<T>::Pointer Pointer;

    MpscQueue()
        : tail_(&stub_)
        , head_(&stub_) {
    }

    ~MpscQueue() {
        // release ownership of all objects
        while (pop_front()) {
        }
    }

    //! Add object to the end of the queue.
    //! Can be called concurrently.
    //! Acquires ownership of @p obj.
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
        Ownership<T>::acquire(obj);

        MpscQueueNode::MpscQueueData* node = obj.mpsc_queue_data();

        if (node->queue) {
            roc_panic("mpsc queue: attempt to push back node more than once");
        }
        node->queue = this;

        push_node_(node);
    }

    //! Remove object from the beginning of the queue (non-blocking version).
    //! Should NOT be called concurrently.
    //! Releases ownership of the returned object.
    //! @remarks
    //!  - Returns NULL if the queue is empty.
    //!  - May return NULL if a concurrent push_back() call is running, even if the
    //!    queue is not empty.
    //! @note
    //!  - This operation is both lock-free and wait-free on all architectures, i.e. it
    //!    never waits for sleeping threads and never spins indefinitely.
    Pointer try_pop_front() {
        MpscQueueNode::MpscQueueData* node = pop_node_<false>();
        if (!node) {
            return NULL;
        }

        node->queue = NULL;

        Pointer obj = static_cast<T*>(node->container_of());
        Ownership<T>::release(*obj);

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
    Pointer pop_front() {
        MpscQueueNode::MpscQueueData* node = pop_node_<true>();
        if (!node) {
            return NULL;
        }

        node->queue = NULL;

        Pointer obj = static_cast<T*>(node->container_of());
        Ownership<T>::release(*obj);

        return obj;
    }

private:
    typedef MpscQueueNode::MpscQueueData MpscQueueData;

    void push_node_(MpscQueueData* node) {
        node->next.store_relaxed(NULL);

        MpscQueueData* prev = tail_.exchange_acq_rel(node);

        prev->next.store_release(node);
    }

    template <bool CanSpin> MpscQueueData* pop_node_() {
        MpscQueueData* head = head_.load_relaxed();
        MpscQueueData* next = head->next.load_acquire();

        if (head == &stub_) {
            if (!next) {
                if (tail_.load_acquire() == head) {
                    // queue is empty
                    return NULL;
                } else {
                    // queue is not empty, so head->next == NULL means that
                    // a push_node_() call is in progress
                    if (!(next = (CanSpin ? wait_next_(head) : try_wait_next_(head)))) {
                        // this may happen only if CanSpin is false
                        return NULL;
                    }
                }
            }
            // remove stub from the beginning of the list
            head_.store_relaxed(next);
            head = next;
            next = next->next.load_acquire();
        }

        if (!next) {
            // head is not stub and head->next == NULL

            if (tail_.load_acquire() == head) {
                // queue is empty
                // add stub to the end of the list to ensure that we always
                // have head->next when removing head and head wont become NULL
                push_node_(&stub_);
            }

            // if head->next == NULL here means that a push_node_() call is in progress
            if (!(next = (CanSpin ? wait_next_(head) : try_wait_next_(head)))) {
                // this may happen only if CanSpin is false
                return NULL;
            }
        }

        // move list head to the next node
        head_.store_relaxed(next);

        return head;
    }

    // Wait until concurrent push_node_() completes and node->next becomes non-NULL.
    // This version may block indefinetely.
    // Usually it returns immediately. It can block only if the thread performing
    // push_node_() was interrupted exactly after updating tail and before updating
    // next, and is now sleeping. In this rare case, this method will wait until the
    // push_node_() thread is resumed and completed.
    MpscQueueData* wait_next_(MpscQueueData* node) {
        for (;;) {
            if (MpscQueueData* next = node->next.load_acquire()) {
                return next;
            }
            cpu_relax();
        }
    }

    // Wait until concurrent push_node_() completes and node->next becomes non-NULL.
    // This version is non-blocking and gives up after a few re-tries.
    // Usually it succeedes. It can fail only in the same rare case when
    // wait_next_() blocks.
    MpscQueueData* try_wait_next_(MpscQueueData* node) {
        MpscQueueData* next;
        if ((next = node->next.load_acquire())) {
            return next;
        }
        if ((next = node->next.load_acquire())) {
            return next;
        }
        if ((next = node->next.load_acquire())) {
            return next;
        }
        return NULL;
    }

    Atomic<MpscQueueData*> tail_;
    Atomic<MpscQueueData*> head_;

    MpscQueueData stub_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MPSC_QUEUE_H_
