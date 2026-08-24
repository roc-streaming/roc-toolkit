/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/pairing_heap_node.h
//! @brief pairing heap node.

#ifndef ROC_CORE_PAIRING_HEAP_NODE_H_
#define ROC_CORE_PAIRING_HEAP_NODE_H_

#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Base class for pairing heap element.
//! @remarks
//!  Object should inherit this class to be able to be a member of PairingHeap.
class PairingHeapNode : public NonCopyable<PairingHeapNode> {
public:
    //! PairingHeap node data.
    struct PairingHeapNodeData {
        //! Left Most Child pairing heap element.
        PairingHeapNodeData* left;

        //! Right Most Child pairing heap element.
        PairingHeapNodeData* leftmost_child;

        //! Previous Sibling pairing heap element.
        PairingHeapNodeData* prev;

        //! Next Sibling pairing heap element.
        PairingHeapNodeData* next;

        //! The pairing heap this node is member of.
        //! @remarks
        //!  NULL if node is not member of any pairing heap.
        void* pairing_heap;

        PairingHeapNodeData()
            : leftmost_child(NULL)
            , prev(NULL)
            , next(NULL)
            , pairing_heap(NULL) {
        }

        //! Get PairingHeapNode object that contains this PairingHeapData object.
        PairingHeapNode* container_of() {
            return ROC_CONTAINER_OF(this, PairingHeapNode, pairing_heap_data_);
        }
    };

    ~PairingHeapNode() {
        if (pairing_heap_data_.pairing_heap != NULL) {
            roc_panic("pairing heap node: can't call destructor for an element that is "
                      "still in pairing heap");
        }
    }

    //! Get pairing heap node data.
    PairingHeapNodeData* pairing_heap_node_data() const {
        return &pairing_heap_data_;
    }

private:
    mutable PairingHeapNodeData pairing_heap_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_PAIRING_HEAP_NODE_H_
