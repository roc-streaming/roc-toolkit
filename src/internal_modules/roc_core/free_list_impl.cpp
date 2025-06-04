/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * Copyright (c) 2014 Cameron Desrochers
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS “AS IS”
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "roc_core/free_list_impl.h"
#include "roc_core/atomic_ops.h"
#include "roc_core/free_list_node.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

static const uint32_t SHOULD_BE_ON_FREELIST = 0x80000000;
static const uint32_t REFS_MASK = 0x7FFFFFFF;

FreeListImpl::FreeListImpl()
    : head_(nullptr) {
}

FreeListImpl::~FreeListImpl() {
}

bool FreeListImpl::is_empty() {
    FreeListData* current_head = AtomicOps::load_acquire(head_);
    return current_head == nullptr;
}

FreeListData* FreeListImpl::unsafe_pop_front() {
    if (!head_) {
        return nullptr;
    }
    FreeListData* node = head_;
    head_ = node->next;
    return node;
}

FreeListData* FreeListImpl::pop_front() {
    FreeListData* current_head = AtomicOps::load_acquire(head_);

    while (current_head != nullptr) {
        FreeListData* prev_head = current_head;

        uint32_t refs = AtomicOps::load_relaxed(current_head->refs);

        if ((refs & REFS_MASK) == 0
            || !AtomicOps::compare_exchange_acquire(current_head->refs, refs, refs + 1)) {
            current_head = AtomicOps::load_acquire(head_);
            continue;
        }

        // Good, reference count has been incremented (it wasn't at zero), which means
        // we can read the next and not worry about it changing between now and the time
        // we do the CAS
        FreeListData* next = AtomicOps::load_relaxed(current_head->next);

        if (AtomicOps::compare_exchange_acquire_relaxed(head_, current_head, next)) {
            // Got the node. This means it was on the list, which means
            // shouldBeOnFreeList must be false no matter the refcount (because
            // nobody else knows it's been taken off yet, it can't have been put back on).
            roc_panic_if_not(
                (AtomicOps::load_relaxed(current_head->refs) & SHOULD_BE_ON_FREELIST)
                == 0);

            // Decrease refcount twice, once for our ref, and once for the list's ref
            AtomicOps::fetch_sub_relaxed(current_head->refs, 2u);

            return current_head;
        }

        // OK, the head must have changed on us, but we still need to decrease the
        // refcount we increased
        if (AtomicOps::fetch_sub_acq_rel(prev_head->refs, 1u)
            == SHOULD_BE_ON_FREELIST + 1) {
            add_knowing_refcount_is_zero_(prev_head);
        }
    }

    return nullptr;
}

void FreeListImpl::push_front(FreeListData* node) {
    // We know that the should-be-on-freelist bit is 0 at this point, so it's safe to
    // set it using a fetch_add
    if (AtomicOps::fetch_add_release(node->refs, SHOULD_BE_ON_FREELIST) == 0) {
        // Oh look! We were the last ones referencing this node, and we know
        // we want to add it to the free list, so let's do it!
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
        AtomicOps::store_release(node->refs, 1u);

        if (!AtomicOps::compare_exchange_release_relaxed(head_, current_head, node)) {
            // Hmm, the add failed, but we can only try again when the refcount goes back
            // to zero
            if (AtomicOps::fetch_add_acq_rel(node->refs, SHOULD_BE_ON_FREELIST - 1)
                == 1) {
                continue;
            }
        }
        return;
    }
}

} // namespace core
} // namespace roc
