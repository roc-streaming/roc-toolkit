/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/list_node.h
//! @brief Linked list node.

#ifndef ROC_CORE_LIST_NODE_H_
#define ROC_CORE_LIST_NODE_H_

#include "roc_core/helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Base class for list element.
//!
//! Object should inherit this class to be able to be a member
//! of List.
class ListNode : public NonCopyable<ListNode> {
public:
    virtual ~ListNode();

    //! List node data.
    struct Node {
        //! Previous list element.
        Node* prev;

        //! Next list element.
        Node* next;

        //! The list this node is member of.
        //!
        //! NULL if node is not member of any list.
        void* list;

        Node()
            : prev(NULL)
            , next(NULL)
            , list(NULL) {
        }

        //! Get container object of this node.
        ListNode* container() {
            return ROC_CONTAINER_OF(this, ListNode, node_);
        }
    };

    //! Get list node data.
    Node* listnode() const {
        return &node_;
    }

private:
    mutable Node node_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_NODE_H_
