/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/mpsc_queue_node.h
//! @brief MpscQueue node.

#ifndef ROC_CORE_MPSC_QUEUE_NODE_H_
#define ROC_CORE_MPSC_QUEUE_NODE_H_

#include "roc_core/atomic.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! MpscQueue node internal data.
struct MpscQueueData {
    //! Next list element.
    MpscQueueData* next;

    //! Pointer to the containing queue.
    void* queue;

    MpscQueueData()
        : next(NULL)
        , queue(NULL) {
    }
};

//! Base class for MpscQueue element.
//! @remarks
//!  Object should inherit this class to be able to be a member of MpscQueue.
//!  Tag allows to inherit multiple copies of ListNode and include same
//!  object into multiple lists.
template <class Tag = void>
class MpscQueueNode : public NonCopyable<MpscQueueNode<Tag> > {
public:
    ~MpscQueueNode() {
        if (mpsc_queue_data_.queue) {
            roc_panic("mpsc node: attempt to destroy node while it's still in queue");
        }
    }

    //! Get pointer to parent node from pointer to internal data.
    static MpscQueueNode* mpsc_queue_node(MpscQueueData* data) {
        return ROC_CONTAINER_OF(data, MpscQueueNode, mpsc_queue_data_);
    }

    //! Get pointer to internal data.
    MpscQueueData* mpsc_queue_data() const {
        return &mpsc_queue_data_;
    }

private:
    mutable MpscQueueData mpsc_queue_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MPSC_QUEUE_NODE_H_
