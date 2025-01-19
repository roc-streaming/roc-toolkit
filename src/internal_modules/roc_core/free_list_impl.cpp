/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/free_list_impl.h"
#include "roc_core/free_list_node.h"
#include "roc_core/panic.h"

namespace roc {
namespace core {

static const uint32_t SHOULD_BE_ON_FREELIST = 0x80000000;
static const uint32_t REFS_MASK = 0x7FFFFFFF;
static const uint32_t SUB_1 = 0xFFFFFFFF;
static const uint32_t SUB_2 = 0xFFFFFFFE;

FreeListImpl::FreeListImpl() {
    head_->next = NULL;
}

FreeListImpl::~FreeListImpl() {
    while (head_ != NULL) {
        unsafe_pop_front();
    }
}

FreeListData* FreeListImpl::try_pop_front() {
    FreeListData* current_head = head_;
    if (current_head == NULL) {
        return NULL;
    }
    while (current_head != NULL) {
        FreeListData* prev_head = current_head;
        uint32_t refs = current_head->refs;
        if ((refs & REFS_MASK) == 0
            || !current_head->refs.compare_exchange(refs, refs + 1)) {
            current_head = head_;
            continue;
        }
        FreeListData* next = current_head->next;
        if (head_.compare_exchange(current_head, next)) {
            if (!((current_head->refs & SHOULD_BE_ON_FREELIST) == 0)) {
                roc_panic("ABA problem");
            }
            current_head->refs += SUB_2;
            return current_head;
        }
        refs = prev_head->refs.postfix_add(SUB_1);
        if (refs == SHOULD_BE_ON_FREELIST + 1) {
            add_knowing_refcount_is_zero_(prev_head);
        }
    }
    return NULL;
}

void FreeListImpl::push_front(FreeListData* node) {
    if (node->refs.postfix_add(SHOULD_BE_ON_FREELIST) == 0) {
        add_knowing_refcount_is_zero_(node);
    }
}

void FreeListImpl::add_knowing_refcount_is_zero_(FreeListData* node) {
    FreeListData* current_head = head_;
    while (true) {
        node->next = current_head;
        node->refs = 1;
        if (!head_.compare_exchange(current_head, node)) {
            if ((node->refs.postfix_add(SHOULD_BE_ON_FREELIST - 1)) == 1)
                continue;
        }
        return;
    }
}
} // namespace core
} // namespace roc
