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

#include "roc_core/macros.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Base class for list element.
//! @remarks
//!  Object should inherit this class to be able to be a member of List.
class ListNode : public NonCopyable<ListNode> {
public:
    //! List node data.
    struct ListData {
        //! Previous list element.
        ListData* prev;

        //! Next list element.
        ListData* next;

        //! The list this node is member of.
        //! @remarks
        //!  NULL if node is not member of any list.
        void* list;

        ListData()
            : prev(NULL)
            , next(NULL)
            , list(NULL) {
        }

        //! Get ListNode object that contains this ListData object.
        ListNode* list_node() {
            return ROC_CONTAINER_OF(this, ListNode, list_data_);
        }
    };

    ~ListNode();

    //! Get list node data.
    ListData* list_data() const {
        return &list_data_;
    }

private:
    mutable ListData list_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_NODE_H_
