/*
 * Copyright (c) 2015 Roc Streaming authors
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
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive doubly-linked list implementation class.
//! Handles List infrastructure independent of templated type for List.
//! Ownership handling is left to the main List class.
class ListImpl : public NonCopyable<> {
public:
    ListImpl();
    ~ListImpl();

    //! Get number of nodes in list.
    size_t size() const;

    //! Check if node belongs to list.
    bool contains(const ListData* node) const;

    //! Get list head (non-node node).
    ListData* head();

    //! Get first list node.
    ListData* front() const;

    //! Get last list node.
    ListData* back() const;

    //! Get list node next to given one.
    ListData* nextof(ListData* node) const;

    //! Get list node previous to given one.
    ListData* prevof(ListData* node) const;

    //! Remove first node and return.
    ListData* pop_front();

    //! Remove last node and return.
    ListData* pop_back();

    //! Insert node into list.
    void insert(ListData* node, ListData* before);

    //! Remove node from list.
    void remove(ListData* node);

private:
    static void check_is_member_(const ListData* node, const ListImpl* list);

    ListData head_;
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_IMPL_H_
