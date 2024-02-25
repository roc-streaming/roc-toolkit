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

//!	Intrusive doubly-linked list implementation class.
//! Ownership is left up to the main List class.

class ListImpl {
public:
    //! Initialize empty list.
    ListImpl();

    //! Get number of elements in list.
    size_t size() const;

    //! Check if size is zero.
    bool is_empty() const;

    //! Check whether list node data is registered in list.
    bool contains(const ListNode::ListNodeData* data);

    //! Get first list element.
    //! @returns
    //!  first element or NULL if list is empty.
    ListNode* front() const;

    //! Get last list element.
    //! @returns
    //!  last element or NULL if list is empty.
    ListNode* back() const;

    //! Get list element next to list node data.
    //!
    //! @returns
    //!  list element following @p data if @p data is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p data must be registered in this list.
    ListNode* nextof(ListNode::ListNodeData* data) const;

    //! Get list element previous to list node data.
    //!
    //! @returns
    //!  element preceeding list node data @p data if element is not
    //!  first, or NULL otherwise.
    //!
    //! @pre
    //! @p data must be registered in this list.
    ListNode* prevof(ListNode::ListNodeData* data) const;

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

    //! Get element to which list node data @p data belongs.
    static ListNode* container_of(ListNode::ListNodeData* data);

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
