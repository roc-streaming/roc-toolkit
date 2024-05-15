/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/list_impl.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

ListImpl::ListImpl()
    : size_(0) {
    head_.prev = &head_;
    head_.next = &head_;
    head_.list = this;
}

ListImpl::~ListImpl() {
    head_.list = NULL;
}

size_t ListImpl::size() const {
    return size_;
}

bool ListImpl::contains(const ListData* node) const {
    return (node->list == this);
}

ListData* ListImpl::head() {
    return &head_;
}

ListData* ListImpl::front() const {
    if (size_ == 0) {
        return NULL;
    }
    return head_.next;
}

ListData* ListImpl::back() const {
    if (size_ == 0) {
        return NULL;
    }
    return head_.prev;
}

ListData* ListImpl::nextof(ListData* node) const {
    check_is_member_(node, this);

    if (node->next == &head_) {
        return NULL;
    }
    return node->next;
}

ListData* ListImpl::prevof(ListData* node) const {
    check_is_member_(node, this);

    if (node->prev == &head_) {
        return NULL;
    }
    return node->prev;
}

ListData* ListImpl::pop_front() {
    if (size_ == 0) {
        roc_panic("list: is empty");
    }

    ListData* node = head_.next;
    remove(node);
    return node;
}

ListData* ListImpl::pop_back() {
    if (size_ == 0) {
        roc_panic("list: is empty");
    }

    ListData* node = head_.prev;
    remove(node);
    return node;
}

void ListImpl::insert(ListData* node_new, ListData* node_before) {
    check_is_member_(node_new, NULL);
    check_is_member_(node_before, this);

    node_new->next = node_before;
    node_new->prev = node_before->prev;

    node_before->prev->next = node_new;
    node_before->prev = node_new;

    node_new->list = this;

    size_++;
}

void ListImpl::remove(ListData* node) {
    check_is_member_(node, this);

    node->prev->next = node->next;
    node->next->prev = node->prev;

    node->list = NULL;

    size_--;
}

void ListImpl::check_is_member_(const ListData* node, const ListImpl* list) {
    if (node->list != list) {
        roc_panic("list: element is member of wrong list: expected %p, got %p",
                  (const void*)list, (const void*)node->list);
    }
}

} // namespace core
} // namespace roc
