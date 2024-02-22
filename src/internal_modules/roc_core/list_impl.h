/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/list_impl.h
//! @brief Intrusive doubly-linked list.

#ifndef ROC_CORE_LIST_IMPL_H_
#define ROC_CORE_LIST_IMPL_H_

#include "roc_core/list_node.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

class ListImpl {
public:
    //! Initialize empty list.
    ListImpl();

    //! Get number of elements in list.
    size_t size() const;

    //! Check if size is zero.
    bool is_empty() const;

    //! Check if data of element belongs to list.
    bool contains_(const ListNode::ListNodeData* data);

    //! Get first list element.
    //! @returns
    //!  first element or NULL if list is empty.
    ListNode* front() const;

    //! Get last list element.
    //! @returns
    //!  last element or NULL if list is empty.
    ListNode* back() const;

    //! Get list element next to given one.
    //!
    //! @returns
    //!  list element following @p element if @p element is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this list.
    ListNode* nextof(ListNode::ListNodeData* data) const;

    //! Get list element previous to given one.
    //!
    //! @returns
    //!  list element preceeding @p element if @p element is not
    //!  first, or NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this list.
    ListNode* prevof_(ListNode::ListNodeData* data) const;

    void insert(ListNode::ListNodeData* data_new, ListNode::ListNodeData* data_before);

    //! Remove element from list.
    //!
    //! @remarks
    //!  - removes @p element from list
    //!  - releases ownership of @p element
    //!
    //! @pre
    //!  @p element should be member of this list.
    void remove(ListNode::ListNodeData* data);

    static ListNode* container_of_(ListNode::ListNodeData* data);

    ListNode::ListNodeData head_;

    static void check_is_member(const ListNode::ListNodeData* data, const ListImpl* list);

private:
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_IMPL_H_
