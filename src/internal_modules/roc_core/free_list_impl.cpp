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
#include "roc_core/target_libatomic_ops/roc_core/atomic_ops.h"

namespace roc {
namespace core {

static const uint32_t SHOULD_BE_ON_FREELIST = 0x80000000;
static const uint32_t REFS_MASK = 0x7FFFFFFF;
static const uint32_t SUB_1 = 0xFFFFFFFF;
static const uint32_t SUB_2 = 0xFFFFFFFE;

FreeListImpl::FreeListImpl()
    : head_(NULL) {
}

FreeListImpl::~FreeListImpl() {
}

FreeListData* FreeListImpl::front() const {
    return head_;
}

bool FreeListImpl::is_empty() {
    if (head_ == NULL)
        return true;
    return false;
}

FreeListData* FreeListImpl::unsafe_pop_front() {
    if (head_ == NULL) {
        return NULL;
    }
    FreeListData* node = head_;
    head_ = node->next;
    return node;
}

FreeListData* FreeListImpl::try_pop_front() {
    FreeListData* current_head = AtomicOps::load_acquire(head_);
    if (current_head == NULL) {
        return NULL;
    }
    while (current_head != NULL) {
        FreeListData* prev_head = current_head;
        uint32_t refs = AtomicOps::load_relaxed(current_head->refs);
        if ((refs & REFS_MASK) == 0
            || !AtomicOps::compare_exchange_acq_rel(head_->refs, current_head->refs,
                                                    refs)) {
            current_head = AtomicOps::load_acquire(head_);
            continue;
        }
        // Good, reference count has been incremented (it wasn't at zero), which means
        // we can read the next and not worry about it changing between now and the time
        // we do the CAS
        FreeListData* next = AtomicOps::load_relaxed(current_head->next);
        if (AtomicOps::compare_exchange_acq_rel(head_, current_head, next)) {
            // Got the node. This means it was on the list, which means
            // shouldBeOnFreeList must be false no matter the refcount (because
            // nobody else knows it's been taken off yet, it can't have been put back on).
            if (!((current_head->refs & SHOULD_BE_ON_FREELIST) == 0)) {
                roc_panic("ABA problem");
            }
            // Decrease refcount twice, once for our ref, and once for the list's ref
            AtomicOps::fetch_add_relaxed(current_head->refs, SUB_2);
            return current_head;
        }
        // OK, the head must have changed on us, but we still need to decrease the
        // refcount we increased
        refs = AtomicOps::fetch_add_acquire(prev_head->refs, SUB_1);
        if (refs == SHOULD_BE_ON_FREELIST + 1) {
            add_knowing_refcount_is_zero_(prev_head);
        }
    }
    return NULL;
}

void FreeListImpl::push_front(FreeListData* node) {
    if (AtomicOps::fetch_add_release(node->refs, SHOULD_BE_ON_FREELIST) == 0) {
        add_knowing_refcount_is_zero_(node);
    }
}

void FreeListImpl::add_knowing_refcount_is_zero_(FreeListData* node) {
    // Since the refcount is zero, and nobody can increase it once it's zero (except us,
    // and we run only one copy of this method per node at a time, i.e. the single thread
    // case), then we know we can safely change the next pointer of the node; however,
    // once the refcount is back above zero, then other threads could increase it (happens
    // under heavy contention, when the refcount goes to zero in between a load and a
    // refcount increment of a node in try_get, then back up to something non-zero, then
    // the refcount increment is done by the other thread) -- so, if the CAS to add the
    // node to the actual list fails, decrease the refcount and leave the add operation to
    // the next thread who puts the refcount back at zero (which could be us, hence the
    // loop).
    FreeListData* current_head = AtomicOps::load_relaxed(head_);
    while (true) {
        AtomicOps::store_relaxed(node->next, current_head);
        AtomicOps::store_release(node->refs, static_cast<uint32_t>(1));
        if (!AtomicOps::compare_exchange_release(head_, current_head, node)) {
            // Hmm, the add failed, but we can only try again when the refcount goes back
            // to zero
            if (AtomicOps::fetch_add_release(node->refs, SHOULD_BE_ON_FREELIST - 1)
                == 1) {
                continue;
            }
        }
        return;
    }
}
} // namespace core
} // namespace roc
