/*
 * Copyright (c) 2015 Roc authors
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
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Base class for list element.
//! @remarks
//!  Object should inherit this class to be able to be a member of List.
class ListNode : public NonCopyable<ListNode> {
public:
    //! List node data.
    struct ListNodeData {
        //! Previous list element.
        ListNodeData* prev;

        //! Next list element.
        ListNodeData* next;

        //! The list this node is member of.
        //! @remarks
        //!  NULL if node is not member of any list.
        void* list;

        ListNodeData()
            : prev(NULL)
            , next(NULL)
            , list(NULL) {
        }

        //! Get ListNode object that contains this ListData object.
        ListNode* container_of() {
            return ROC_CONTAINER_OF(this, ListNode, list_data_);
        }
    };

    ~ListNode() {
        if (list_data_.list != NULL) {
            roc_panic(
                "list node: can't call destructor for an element that is still in list");
        }
    }

    //! Get list node data.
    ListNodeData* list_node_data() const {
        return &list_data_;
    }

private:
    mutable ListNodeData list_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_NODE_H_
