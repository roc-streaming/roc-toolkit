/*
 * Copyright (c) 2020 Roc Streaming authors
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
    void push_back(MpscQueueData* node);

    //! Remove object from the beginning of the queue.
    MpscQueueData* pop_front(bool can_spin);

private:
    void push_node_(MpscQueueData* node);
    MpscQueueData* pop_node_(bool can_spin);

    MpscQueueData* wait_next_(MpscQueueData* node);
    MpscQueueData* try_wait_next_(MpscQueueData* node);

    void change_owner_(MpscQueueData* node, void* from, void* to);

    MpscQueueData* tail_;
    MpscQueueData* head_;

    MpscQueueData stub_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MPSC_QUEUE_IMPL_H_
