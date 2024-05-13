/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/list_node.h
//! @brief Linked list node.

#ifndef ROC_CORE_LIST_NODE_H_
#define ROC_CORE_LIST_NODE_H_

#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! List node internal data.
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
};

//! Base class for List element.
//! @remarks
//!  Object should inherit this class to be able to be a member of List.
//!  Tag allows to inherit multiple copies of ListNode and include same
//!  object into multiple lists.
template <class Tag = void> class ListNode : public NonCopyable<ListNode<Tag> > {
public:
    ~ListNode() {
        if (list_data_.list != NULL) {
            roc_panic("list node: attempt to destroy node while it's still in queue");
        }
    }

    //! Get pointer to parent node from pointer to internal data.
    static ListNode* list_node(ListData* data) {
        return ROC_CONTAINER_OF(data, ListNode, list_data_);
    }

    //! Get pointer to internal data.
    ListData* list_data() const {
        return &list_data_;
    }

private:
    mutable ListData list_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_NODE_H_
