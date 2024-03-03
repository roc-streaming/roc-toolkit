/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/list_impl.h"
#include "roc_core/list_node.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

ListImpl::ListImpl()
    : size_(0) {
    head.prev = &head;
    head.next = &head;
    head.list = this;
}

size_t ListImpl::size() const {
    return size_;
}

bool ListImpl::contains(const ListNode::ListNodeData* data) const {
    return (data->list == this);
}

ListNode::ListNodeData* ListImpl::front() const {
    if (size_ == 0) {
        return NULL;
    }
    return head.next;
}

ListNode::ListNodeData* ListImpl::back() const {
    if (size_ == 0) {
        return NULL;
    }
    return head.prev;
}

ListNode::ListNodeData* ListImpl::nextof(ListNode::ListNodeData* data) const {
    check_is_member(data, this);

    if (data->next == &head) {
        return NULL;
    }
    return data->next;
}

ListNode::ListNodeData* ListImpl::prevof(ListNode::ListNodeData* data) const {
    check_is_member(data, this);

    if (data->prev == &head) {
        return NULL;
    }
    return data->prev;
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
        roc_panic(
            "list: list node data is not registered in this list: expected %p, got %p",
            (const void*)list, (const void*)data->list);
    }
}

} // namespace core
} // namespace roc
