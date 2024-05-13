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

#include "roc_core/list_node.h"
#include "roc_core/noncopyable.h"
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
    List()
        : size_(0) {
        head_.prev = &head_;
        head_.next = &head_;
        head_.list = this;
    }

    //! Release ownership of containing objects.
    ~List() {
        ListData* next_data;

        for (ListData* data = head_.next; data != &head_; data = next_data) {
            roc_panic_if(data == NULL);
            check_is_member_(data, this);

            next_data = data->next;
            data->list = NULL;

            OwnershipPolicy<T>::release(*from_node_data_(data));
        }

        head_.list = NULL;
    }

    //! Get number of elements in list.
    size_t size() const {
        return size_;
    }

    //! Check if size is zero.
    bool is_empty() const {
        return size_ == 0;
    }

    //! Check if element belongs to list.
    bool contains(const T& elem) {
        const ListData* data = to_node_data_(elem);
        return (data->list == this);
    }

    //! Get first list element.
    //! @returns
    //!  first element or NULL if list is empty.
    Pointer front() const {
        if (size_ == 0) {
            return NULL;
        }
        return from_node_data_(head_.next);
    }

    //! Get last list element.
    //! @returns
    //!  last element or NULL if list is empty.
    Pointer back() const {
        if (size_ == 0) {
            return NULL;
        }
        return from_node_data_(head_.prev);
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
        check_is_member_(data, this);

        if (data->next == &head_) {
            return NULL;
        }
        return from_node_data_(data->next);
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
        check_is_member_(data, this);

        if (data->prev == &head_) {
            return NULL;
        }
        return from_node_data_(data->prev);
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
        insert_(elem, head_.next);
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
        insert_(elem, &head_);
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
        if (size_ == 0) {
            roc_panic("list: is empty");
        }
        remove_(*from_node_data_(head_.next));
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
        if (size_ == 0) {
            roc_panic("list: is empty");
        }
        remove_(*from_node_data_(head_.prev));
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
        insert_(elem, to_node_data_(before));
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
        insert_(elem, to_node_data_(after)->next);
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
        remove_(elem);
    }

private:
    static ListData* to_node_data_(const T& elem) {
        return static_cast<const Node&>(elem).list_data();
    }

    static T* from_node_data_(ListData* data) {
        return static_cast<T*>(static_cast<Node*>(Node::list_node(data)));
    }

    static void check_is_member_(const ListData* data, const List* list) {
        if (data->list != list) {
            roc_panic("list: element is member of wrong list: expected %p, got %p",
                      (const void*)list, (const void*)data->list);
        }
    }

    void insert_(T& elem, ListData* data_before) {
        ListData* data_new = to_node_data_(elem);
        check_is_member_(data_new, NULL);
        check_is_member_(data_before, this);

        data_new->next = data_before;
        data_new->prev = data_before->prev;

        data_before->prev->next = data_new;
        data_before->prev = data_new;

        data_new->list = this;

        size_++;

        OwnershipPolicy<T>::acquire(elem);
    }

    void remove_(T& elem) {
        ListData* data = to_node_data_(elem);
        check_is_member_(data, this);

        data->prev->next = data->next;
        data->next->prev = data->prev;

        data->list = NULL;

        size_--;

        OwnershipPolicy<T>::release(elem);
    }

    ListData head_;
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_H_
