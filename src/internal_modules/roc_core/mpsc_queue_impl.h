/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/mpsc_queue_impl.h
//! @brief Multi-producer single-consumer queue internal implementation.

#ifndef ROC_CORE_MPSC_QUEUE_IMPL_H_
#define ROC_CORE_MPSC_QUEUE_IMPL_H_

#include "roc_core/mpsc_queue_node.h"

namespace roc {
namespace core {

//! Multi-producer single-consumer queue internal implementation class.
//!
//! Provides only push/pop functionality. Ownership is left up to the main MpscQueue
//! class.
class MpscQueueImpl {
public:
    MpscQueueImpl();

    ~MpscQueueImpl();

    //! Add object to the end of the queue.
    void push_back(MpscQueueNode::MpscQueueData* node);

    //! Remove object from the beginning of the queue.
    template <bool CanSpin> MpscQueueNode::MpscQueueData* pop_front() {
        return pop_node_(CanSpin);
    }

private:
    typedef MpscQueueNode::MpscQueueData MpscQueueData;

    void push_node_(MpscQueueData* node);

    MpscQueueData* pop_node_(bool can_spin);

    // Wait until concurrent push_node_() completes and node->next becomes non-NULL.
    // This version may block indefinetely.
    // Usually it returns immediately. It can block only if the thread performing
    // push_node_() was interrupted exactly after updating tail and before updating
    // next, and is now sleeping. In this rare case, this method will wait until the
    // push_node_() thread is resumed and completed.
    MpscQueueData* wait_next_(MpscQueueData* node);

    // Wait until concurrent push_node_() completes and node->next becomes non-NULL.
    // This version is non-blocking and gives up after a few re-tries.
    // Usually it succeeds. It can fail only in the same rare case when
    // wait_next_() blocks.
    MpscQueueData* try_wait_next_(MpscQueueData* node);

    MpscQueueData* tail_;
    MpscQueueData* head_;

    MpscQueueData stub_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MPSC_QUEUE_IMPL_H_
