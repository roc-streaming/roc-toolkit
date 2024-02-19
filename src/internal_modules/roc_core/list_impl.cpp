/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/list.h
//! @brief Intrusive doubly-linked list.

#include "roc_core/list_impl.h"
#include "roc_core/list_node.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive doubly-linked list.
//!
//! Does not perform allocations.
//! Provides O(1) size check, membership check, insertion, and removal.
//!
//! @tparam T defines object type, it should inherit ListNode.
//!
//! @tparam OwnershipPolicy defines ownership policy which is used to acquire an
//! element ownership when it's added to the list and release ownership when it's
//! removed from the list.

//! Initialize empty list.
ListImpl::ListImpl()
    : size_(0) {
    head_.prev = &head_;
    head_.next = &head_;
    head_.list = this;
}

//! Get number of elements in list.
size_t ListImpl::size() const {
    return size_;
}

//! Check if size is zero.
bool ListImpl::is_empty() const {
    return size_ == 0;
}

//! Check if element belongs to list.
bool ListImpl::contains_(const ListNode::ListNodeData* data) {
    return (data->list == this);
}

//! Get first list element.
//! @returns
//!  first element or NULL if list is empty.
ListNode* ListImpl::front() const {
    if (size_ == 0) {
        return NULL;
    }
    return container_of_(head_.next);
}

//! Get last list element.
//! @returns
//!  last element or NULL if list is empty.
ListNode* ListImpl::back() const {
    if (size_ == 0) {
        return NULL;
    }
    return container_of_(head_.prev);
}

//! Get list element next to given one.
//!
//! @returns
//!  list element following @p element if @p element is not
//!  last, or NULL otherwise.
//!
//! @pre
//!  @p element should be member of this list.
ListNode* ListImpl::nextof_(ListNode::ListNodeData* data) const {
    check_is_member(data, this);

    if (data->next == &head_) {
        return NULL;
    }
    return container_of_(data->next);
}

//! Get list element previous to given one.
//!
//! @returns
//!  list element preceeding @p element if @p element is not
//!  first, or NULL otherwise.
//!
//! @pre
//!  @p element should be member of this list.
ListNode* ListImpl::prevof_(ListNode::ListNodeData* data) const {
    check_is_member(data, this);

    if (data->prev == &head_) {
        return NULL;
    }
    return container_of_(data->prev);
}

ListNode* ListImpl::container_of_(ListNode::ListNodeData* data) {
    return data->container_of();
}

void ListImpl::insert(ListNode::ListNodeData* data_new,
                      ListNode::ListNodeData* data_before) {
    check_is_member(data_new, NULL);
    check_is_member(data_before, this);

    data_new->next = data_before;
    data_new->prev = data_before->prev;

    data_before->prev->next = data_new;
    data_before->prev = data_new;

    data_new->list = this;

    size_++;
}

void ListImpl::remove(ListNode::ListNodeData* data) {
    check_is_member(data, this);

    data->prev->next = data->next;
    data->next->prev = data->prev;

    data->list = NULL;

    size_--;
}

void ListImpl::check_is_member(const ListNode::ListNodeData* data, const ListImpl* list) {
    if (data->list != list) {
        roc_panic("list: element is member of wrong list: expected %p, got %p",
                  (const void*)list, (const void*)data->list);
    }
}

} // namespace core
} // namespace roc
