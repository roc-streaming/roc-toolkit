/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/list.h
//! @brief Intrusive doubly-linked list.

#ifndef ROC_CORE_LIST_H_
#define ROC_CORE_LIST_H_

#include "roc_core/list_impl.h"
#include "roc_core/list_node.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive doubly-linked list.
//!
//! Does not perform allocations.
//! Provides O(1) size check, membership check, insertion, and removal.
//!
//! @tparam T defines object type, it must inherit ListNode.
//!
//! @tparam OwnershipPolicy defines ownership policy which is used to acquire an
//! element ownership when it's added to the list and release ownership when it's
//! removed from the list.
//!
//! @tparam Node defines base class of list nodes. It is needed if ListNode is
//! used with non-default tag.
template <class T,
          template <class TT> class OwnershipPolicy = RefCountedOwnership,
          class Node = ListNode<> >
class List : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename OwnershipPolicy<T>::Pointer Pointer;

    //! Initialize empty list.
    List() {
    }

    //! Release ownership of containing objects.
    ~List() {
        while (!is_empty()) {
            pop_back();
        }
    }

    //! Get number of elements in list.
    size_t size() const {
        return impl_.size();
    }

    //! Check if size is zero.
    bool is_empty() const {
        return impl_.size() == 0;
    }

    //! Check if element belongs to list.
    bool contains(const T& elem) {
        const ListData* data = to_node_data_(elem);
        return impl_.contains(data);
    }

    //! Get first list element.
    //! @returns
    //!  first element or NULL if list is empty.
    Pointer front() const {
        ListData* data = impl_.front();
        return from_node_data_(data);
    }

    //! Get last list element.
    //! @returns
    //!  last element or NULL if list is empty.
    Pointer back() const {
        ListData* data = impl_.back();
        return from_node_data_(data);
    }

    //! Get list element next to given one.
    //!
    //! @returns
    //!  list element following @p elem if @p elem is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p elem should be member of this list.
    Pointer nextof(T& elem) const {
        ListData* data = to_node_data_(elem);
        ListData* next_data = impl_.nextof(data);
        return from_node_data_(next_data);
    }

    //! Get list element previous to given one.
    //!
    //! @returns
    //!  list element preceding @p elem if @p elem is not
    //!  first, or NULL otherwise.
    //!
    //! @pre
    //!  @p elem should be member of this list.
    Pointer prevof(T& elem) const {
        ListData* data = to_node_data_(elem);
        ListData* prev_data = impl_.prevof(data);
        return from_node_data_(prev_data);
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

        ListData* data = to_node_data_(elem);
        impl_.insert(data, impl_.head()->next);
    }

    //! Append element to list.
    //!
    //! @remarks
    //!  - appends @p elem to list
    //!  - acquires ownership of @p elem
    //!
    //! @pre
    //!  @p elem should not be member of any list.
    void push_back(T& elem) {
        OwnershipPolicy<T>::acquire(elem);

        ListData* data = to_node_data_(elem);
        impl_.insert(data, impl_.head());
    }

    //! Pop first element from list.
    //!
    //! @remarks
    //!  - removes first element of list
    //!  - releases ownership of removed element
    //!
    //! @pre
    //!  the list should not be empty.
    void pop_front() {
        ListData* data = impl_.pop_front();
        T* elem = from_node_data_(data);

        OwnershipPolicy<T>::release(*elem);
    }

    //! Pop last element from list.
    //!
    //! @remarks
    //!  - removes last element of list
    //!  - releases ownership of removed element
    //!
    //! @pre
    //!  the list should not be empty.
    void pop_back() {
        ListData* data = impl_.pop_back();
        T* elem = from_node_data_(data);

        OwnershipPolicy<T>::release(*elem);
    }

    //! Insert element into list.
    //!
    //! @remarks
    //!  - inserts @p elem before @p before
    //!  - acquires ownership of @p elem
    //!
    //! @pre
    //!  @p elem should not be member of any list.
    //!  @p before should be member of this list or NULL.
    void insert_before(T& elem, T& before) {
        OwnershipPolicy<T>::acquire(elem);

        ListData* data = to_node_data_(elem);
        ListData* data_before = to_node_data_(before);
        impl_.insert(data, data_before);
    }

    //! Insert element into list.
    //!
    //! @remarks
    //!  - inserts @p elem after @p after
    //!  - acquires ownership of @p elem
    //!
    //! @pre
    //!  @p elem should not be member of any list.
    //!  @p after should be member of this list.
    void insert_after(T& elem, T& after) {
        OwnershipPolicy<T>::acquire(elem);

        ListData* data = to_node_data_(elem);
        ListData* data_after = to_node_data_(after);
        impl_.insert(data, data_after->next);
    }

    //! Remove element from list.
    //!
    //! @remarks
    //!  - removes @p elem from list
    //!  - releases ownership of @p elem
    //!
    //! @pre
    //!  @p elem should be member of this list.
    void remove(T& elem) {
        ListData* data = to_node_data_(elem);
        impl_.remove(data);

        OwnershipPolicy<T>::release(elem);
    }

private:
    static ListData* to_node_data_(const T& elem) {
        return static_cast<const Node&>(elem).list_data();
    }

    static T* from_node_data_(ListData* data) {
        return static_cast<T*>(static_cast<Node*>(Node::list_node(data)));
    }

    ListImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_H_
