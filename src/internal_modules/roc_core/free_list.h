/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/free_list.h
//! @brief Intrusive doubly-linked list.

#ifndef ROC_CORE_FREE_LIST_H_
#define ROC_CORE_FREE_LIST_H_

#include "roc_core/free_list_impl.h"
#include "roc_core/free_list_node.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! A simple CAS-based lock-free free list
//!
//! @tparam T defines object type, it must inherit FreeListNode.
//!
//! @tparam OwnershipPolicy defines ownership policy which is used to acquire an
//! element ownership when it's added to the list and release ownership when it's
//! removed from the list.
//!
//! @tparam Node defines base class of list nodes. It is needed if FreeListNode is
//! used with non-default tag.
template <class T,
          template <class TT> class OwnershipPolicy = RefCountedOwnership,
          class Node = FreeListNode<> >
class FreeList : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename OwnershipPolicy<T>::Pointer Pointer;

    //! Initialize empty list.
    FreeList() {
    }

    //! Release ownership of containing objects.
    ~FreeList() {
        while (!is_empty()) {
            unsafe_pop_front_();
        }
    }

    //! Checks if list is empty
    bool is_empty() {
        return impl_.is_empty();
    }

    //! Prepend element to list.
    //!
    //! @remarks
    //!  - prepends @p elem to list
    //!  - acquires ownership of @p elem
    //!
    //! @pre
    //!  @p elem should not be member of any list.
    void push_front(T& elem) {
        OwnershipPolicy<T>::acquire(elem);

        FreeListData* data = to_free_node_data_(elem);
        impl_.push_front(data);
    }

    //! Pop first element from list.
    //!
    //! @remarks
    //!  - removes first element of list
    //!  - transfers ownership of removed element
    //!
    //! @returns
    //!  element or null if list is empty.
    Pointer pop_front() {
        FreeListData* data = impl_.pop_front();
        Pointer elem = from_free_node_data_(data);

        if (elem) {
            OwnershipPolicy<T>::release(*elem);
        }
        return elem;
    }

private:
    static FreeListData* to_free_node_data_(const T& elem) {
        return static_cast<const Node&>(elem).list_data();
    }

    static T* from_free_node_data_(FreeListData* data) {
        return static_cast<T*>(static_cast<Node*>(Node::list_node(data)));
    }

    void unsafe_pop_front_() {
        FreeListData* data = impl_.unsafe_pop_front();
        T* elem = from_free_node_data_(data);

        OwnershipPolicy<T>::release(*elem);
    }

    FreeListImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_FREE_LIST_H_
