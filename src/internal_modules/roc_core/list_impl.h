/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/list_impl.h
//! @brief Intrusive doubly-linked list implementation.

#ifndef ROC_CORE_LIST_IMPL_H_
#define ROC_CORE_LIST_IMPL_H_

#include "roc_core/list_node.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive doubly-linked list implementation class.
//! Handles List infrastructure independent of templated type for List.
//! Ownership handling is left to the main List class.

class ListImpl {
public:
    //! Initialize empty list.
    ListImpl();

    //! Get number of elements in list.
    size_t size() const;

    //! Check whether list node data is registered in list.
    bool contains(const ListNode::ListNodeData* data) const;

    //! Get first list node.
    //! @returns
    //!  first list node or NULL if list is empty.
    ListNode::ListNodeData* front() const;

    //! Get last list node.
    //! @returns
    //!  last list node or NULL if list is empty.
    ListNode::ListNodeData* back() const;

    //! Get list node next to list node with given list node data.
    //!
    //! @returns
    //!  list node following list node with data @p data if this list node is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p data must be registered in this list.
    ListNode::ListNodeData* nextof(ListNode::ListNodeData* data) const;

    //! Get list node previous to list node with given list node data.
    //!
    //! @returns
    //!  list node preceeding list node with data @p data if this list node is not
    //!  first, or NULL otherwise.
    //!
    //! @pre
    //! @p data must be registered in this list.
    ListNode::ListNodeData* prevof(ListNode::ListNodeData* data) const;

    //! Insert new list node data before given list node data.
    //!
    //! @pre
    //! @p data_before must be registered in this list.
    void insert(ListNode::ListNodeData* data_new, ListNode::ListNodeData* data_before);

    //! Remove list node data from list.
    //!
    //! @pre
    //!  @p data must be registered in this list.
    void remove(ListNode::ListNodeData* data);

    //! Check if list node data is registered in this list.
    static void check_is_member(const ListNode::ListNodeData* data, const ListImpl* list);

    //! Head of list
    ListNode::ListNodeData head;

private:
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_IMPL_H_
