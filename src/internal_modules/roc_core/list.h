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
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive doubly-linked list.
//!
//! Does not perform allocations.
//! Provides O(1) size check, membership check, insertion, and removal.
//!
//! @tparam T defines object type, it must inherit from ListNode.
//!
//! @tparam OwnershipPolicy defines ownership policy which is used to acquire an
//! element ownership when it's added to the list and release ownership when it's
//! removed from the list.
template <class T, template <class TT> class OwnershipPolicy = RefCountedOwnership>
class List : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename OwnershipPolicy<T>::Pointer Pointer;

    //! Release ownership of containing objects.
    ~List() {
        ListNode::ListNodeData* next_data;

        for (ListNode::ListNodeData* data = impl_.head.next; data != &impl_.head;
             data = next_data) {
            roc_panic_if(data == NULL);
            ListImpl::check_is_member(data, &impl_);

            next_data = data->next;
            data->list = NULL;

            OwnershipPolicy<T>::release(*static_cast<T*>(impl_.container_of(data)));
        }

        impl_.head.list = NULL;
    }

    //! Get number of elements in list.
    size_t size() const {
        return impl_.size();
    }

    //! Check if size is zero.
    bool is_empty() const {
        return impl_.is_empty();
    }

    //! Check if element belongs to list.
    bool contains(const T& element) {
        const ListNode::ListNodeData* data = element.list_node_data();
        return impl_.contains(data);
    }

    //! Get first list element.
    //! @returns
    //!  first element or NULL if list is empty.
    Pointer front() const {
        return static_cast<T*>(impl_.front());
    }

    //! Get last list element.
    //! @returns
    //!  last element or NULL if list is empty.
    Pointer back() const {
        return static_cast<T*>(impl_.back());
    }

    //! Get list element next to given one.
    //!
    //! @returns
    //!  list element following @p element if @p element is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this list.
    Pointer nextof(T& element) const {
        ListNode::ListNodeData* data = element.list_node_data();
        return static_cast<T*>(impl_.nextof(data));
    }

    //! Get list element previous to given one.
    //!
    //! @returns
    //!  list element preceeding @p element if @p element is not
    //!  first, or NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this list.
    Pointer prevof(T& element) const {
        ListNode::ListNodeData* data = element.list_node_data();
        return static_cast<T*>(impl_.prevof(data));
    }

    //! Prepend element to list.
    //!
    //! @remarks
    //!  - prepends @p element to list
    //!  - acquires ownership of @p element
    //!
    //! @pre
    //!  @p element should not be member of any list.
    void push_front(T& element) {
        insert_(element, impl_.head.next);
    }

    //! Append element to list.
    //!
    //! @remarks
    //!  - appends @p element to list
    //!  - acquires ownership of @p element
    //!
    //! @pre
    //!  @p element should not be member of any list.
    void push_back(T& element) {
        insert_(element, &impl_.head);
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
        if (size() == 0) {
            roc_panic("list: is empty");
        }
        remove(*static_cast<T*>(impl_.container_of(impl_.head.next)));
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
        if (size() == 0) {
            roc_panic("list: is empty");
        }
        remove(*static_cast<T*>(impl_.container_of(impl_.head.prev)));
    }

    //! Insert element into list.
    //!
    //! @remarks
    //!  - inserts @p element before @p before
    //!  - acquires ownership of @p element
    //!
    //! @pre
    //!  @p element should not be member of any list.
    //!  @p before should be member of this list or NULL.
    void insert_before(T& element, T& before) {
        insert_(element, before.list_node_data());
    }

    //! Insert element into list.
    //!
    //! @remarks
    //!  - inserts @p element after @p after
    //!  - acquires ownership of @p element
    //!
    //! @pre
    //!  @p element should not be member of any list.
    //!  @p after should be member of this list.
    void insert_after(T& element, T& after) {
        insert_(element, after.list_node_data()->next);
    }

    //! Remove element from list.
    //!
    //! @remarks
    //!  - removes @p element from list
    //!  - releases ownership of @p element
    //!
    //! @pre
    //!  @p element should be member of this list.
    void remove(T& element) {
        impl_.remove(element.list_node_data());

        OwnershipPolicy<T>::release(element);
    }

private:
    //! Insert element preceding element with given list node data into list.
    //!
    //! @remarks
    //!  - inserts @p element before @p data_before
    //!  - acquires ownership of @p element
    //!
    //! @pre
    //!  @p element must be member of any list.
    //!  @p data_before must be registered in this list.
    void insert_(T& element, ListNode::ListNodeData* data_before) {
        impl_.insert(element.list_node_data(), data_before);

        OwnershipPolicy<T>::acquire(element);
    }

    ListImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_H_
