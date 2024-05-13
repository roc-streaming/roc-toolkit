/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/mpsc_queue_impl.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/cpu_instructions.h"

namespace roc {
namespace core {

MpscQueueImpl::MpscQueueImpl()
    : tail_(&stub_)
    , head_(&stub_) {
}

MpscQueueImpl::~MpscQueueImpl() {
    if (head_ != &stub_) {
        roc_panic("mpsc queue: queue isn't empty on destruct");
    }
}

void MpscQueueImpl::push_back(MpscQueueData* node) {
    change_owner_(node, NULL, this);
    push_node_(node);
}

MpscQueueData* MpscQueueImpl::pop_front(bool can_spin) {
    MpscQueueData* node = pop_node_(can_spin);
    if (node != NULL) {
        change_owner_(node, this, NULL);
    }
    return node;
}

void MpscQueueImpl::push_node_(MpscQueueData* node) {
    AtomicOps::store_relaxed(node->next, (MpscQueueData*)NULL);
    MpscQueueData* prev = AtomicOps::exchange_seq_cst(tail_, node);
    AtomicOps::store_release(prev->next, node);
}

MpscQueueData* MpscQueueImpl::pop_node_(bool can_spin) {
    MpscQueueData* head = AtomicOps::load_relaxed(head_);
    MpscQueueData* next = AtomicOps::load_acquire(head->next);

    if (head == &stub_) {
        if (!next) {
            if (AtomicOps::load_seq_cst(tail_) == head) {
                // queue is empty
                return NULL;
            } else {
                // queue is not empty, so head->next == NULL means that
                // a push_node_() call is in progress
                if (!(next = (can_spin ? wait_next_(head) : try_wait_next_(head)))) {
                    // this may happen only if can_spin is false
                    return NULL;
                }
            }
        }
        // remove stub from the beginning of the list
        AtomicOps::store_relaxed(head_, next);
        head = next;
        next = AtomicOps::load_acquire(next->next);
    }

    if (!next) {
        // head is not stub and head->next == NULL

        if (AtomicOps::load_seq_cst(tail_) == head) {
            // queue is empty
            // add stub to the end of the list to ensure that we always
            // have head->next when removing head and head wont become NULL
            push_node_(&stub_);
        }

        // if head->next == NULL here means that a push_node_() call is in progress
        if (!(next = (can_spin ? wait_next_(head) : try_wait_next_(head)))) {
            // this may happen only if can_spin is false
            return NULL;
        }
    }

    // move list head to the next node
    AtomicOps::store_relaxed(head_, next);

    return head;
}

// Wait until concurrent push_node_() completes and node->next becomes non-NULL.
// This version may block indefinitely.
// Usually it returns immediately. It can block only if the thread performing
// push_node_() was interrupted exactly after updating tail and before updating
// next, and is now sleeping. In this rare case, this method will wait until the
// push_node_() thread is resumed and completed.
MpscQueueData* MpscQueueImpl::wait_next_(MpscQueueData* node) {
    if (MpscQueueData* next = try_wait_next_(node)) {
        return next;
    }
    for (;;) {
        if (MpscQueueData* next = AtomicOps::load_seq_cst(node->next)) {
            return next;
        }
        cpu_relax();
    }
}

// Wait until concurrent push_node_() completes and node->next becomes non-NULL.
// This version is non-blocking and gives up after a few re-tries.
// Usually it succeeds. It can fail only in the same rare case when
// wait_next_() blocks.
MpscQueueData* MpscQueueImpl::try_wait_next_(MpscQueueData* node) {
    MpscQueueData* next;
    if ((next = AtomicOps::load_acquire(node->next))) {
        return next;
    }
    if ((next = AtomicOps::load_acquire(node->next))) {
        return next;
    }
    if ((next = AtomicOps::load_acquire(node->next))) {
        return next;
    }
    return NULL;
}

void MpscQueueImpl::change_owner_(MpscQueueData* node, void* from, void* to) {
    void* exp = from;
    if (!AtomicOps::compare_exchange_relaxed(node->queue, exp, to)) {
        roc_panic("mpsc queue: unexpected node owner: from=%p to=%p cur=%p", from, to,
                  exp);
    }
}

} // namespace core
} // namespace roc
