/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/free_list_impl.h
//! @brief TODO.

#ifndef ROC_CORE_FREE_LIST_IMPL_H_
#define ROC_CORE_FREE_LIST_IMPL_H_

#include "roc_core/free_list_node.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! A simple CAS-based lock-free free list. Not the fastest thing in the world under heavy
//! contention, but simple and correct (assuming nodes are never freed until after the
//! free list is destroyed), and fairly speedy under low contention. Implemented like a
//! stack, but where node order doesn't matter (nodes are inserted out of order under
//! contention)
//!
//! Credits:
//! Based on the article by Cameron:
//! https://moodycamel.com/blog/2014/solving-the-aba-problem-for-lock-free-free-lists.htm
class FreeListImpl : public NonCopyable<> {
public:
    FreeListImpl();
    ~FreeListImpl();

    //! Check if list is empty
    bool is_empty();

    //! Try to remove first node and return.
    FreeListData* pop_front();

    //! Remove first element under the condition that the list is not being used by anyone
    FreeListData* unsafe_pop_front();

    //! Insert node into list.
    void push_front(FreeListData* node);

private:
    //! Add node knowing that it is not part of a free list.
    void add_knowing_refcount_is_zero_(FreeListData* node);

    FreeListData* head_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_FREE_LIST_IMPL_H_
