/*
 * Copyright (c) 2023 Roc Streaming authors
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

void MpscQueueImpl::push_back(MpscQueueData* node) {
    push_node_(node);
}

void MpscQueueImpl::push_node_(MpscQueueData* node) {
    AtomicOps::store_relaxed(node->next, (MpscQueueData*)NULL);

    MpscQueueData* prev = AtomicOps::exchange_seq_cst(tail_, node);

    AtomicOps::store_release(prev->next, node);
}

MpscQueueImpl::MpscQueueData* MpscQueueImpl::pop_node_(bool can_spin) {
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
                    // this may happen only if CanSpin is false
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
            // this may happen only if CanSpin is false
            return NULL;
        }
    }

    // move list head to the next node
    AtomicOps::store_relaxed(head_, next);

    return head;
}

MpscQueueImpl::MpscQueueData* MpscQueueImpl::wait_next_(MpscQueueData* node) {
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

MpscQueueImpl::MpscQueueData* MpscQueueImpl::try_wait_next_(MpscQueueData* node) {
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

} // namespace core
} // namespace roc
