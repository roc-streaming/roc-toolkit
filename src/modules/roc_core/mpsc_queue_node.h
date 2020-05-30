/*
 * Copyright (c) 2020 Roc authors
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
#include "roc_core/helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

//! MpscQueue node.
class MpscQueueNode : public NonCopyable<MpscQueueNode> {
public:
    //! List node data.
    struct MpscQueueData {
        //! Next list element.
        MpscQueueData* next;

        //! Pointer to the containing queue.
        void* queue;

        MpscQueueData()
            : next(NULL)
            , queue(NULL) {
        }

        //! Get MpscQueueNode object that contains this ListData object.
        MpscQueueNode* container_of() {
            return ROC_CONTAINER_OF(this, MpscQueueNode, mpsc_queue_data_);
        }
    };

    ~MpscQueueNode() {
        if (mpsc_queue_data_.queue) {
            roc_panic("mpsc node: attempt to destroy node while it's still in queue");
        }
    }

    //! Get list node data.
    MpscQueueData* mpsc_queue_data() const {
        return &mpsc_queue_data_;
    }

private:
    mutable MpscQueueData mpsc_queue_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MPSC_QUEUE_NODE_H_
