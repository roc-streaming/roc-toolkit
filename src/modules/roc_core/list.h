/*
 * Copyright (c) 2015 Roc authors
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
#include "roc_core/ownership.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive doubly-linked list.
//!
//! @tparam T defines object type, it should inherit ListNode.
//!
//! @tparam Ownership defines ownership policy which is used to acquire an element
//! ownership when it's added to the list and release ownership when it's removed
//! from the list.
template <class T, template <class TT> class Ownership = RefCntOwnership>
class List : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename Ownership<T>::Pointer Pointer;

    //! Initialize empty list.
    List()
        : size_(0) {
        head_.prev = &head_;
        head_.next = &head_;
        head_.list = this;
    }

    //! Release ownership of containing objects.
    ~List() {
        ListNode::ListNodeData* next_data;

        for (ListNode::ListNodeData* data = head_.next; data != &head_;
             data = next_data) {
            roc_panic_if(data == NULL);
            check_is_member_(data, this);

            next_data = data->next;
            data->list = NULL;

            Ownership<T>::release(*container_of_(data));
        }

        head_.list = NULL;
    }

    //! Get number of elements in list.
    size_t size() const {
        return size_;
    }

    //! Get first list element.
    //! @returns
    //!  first element or NULL if list is empty.
    Pointer front() const {
        if (size_ == 0) {
            return NULL;
        }
        return container_of_(head_.next);
    }

    //! Get last list element.
    //! @returns
    //!  last element or NULL if list is empty.
    Pointer back() const {
        if (size_ == 0) {
            return NULL;
        }
        return container_of_(head_.prev);
    }

    //! Check if element belongs to list.
    bool contains(const T& element) {
        const ListNode::ListNodeData* data = element.list_node_data();
        return (data->list == this);
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
        check_is_member_(data, this);

        if (data->next == &head_) {
            return NULL;
        }
        return container_of_(data->next);
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
        if (size_ == 0) {
            insert_(element, NULL);
        } else {
            insert_(element, container_of_(head_.next));
        }
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
        insert_(element, NULL);
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
        insert_(element, &before);
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
        ListNode::ListNodeData* data = element.list_node_data();
        check_is_member_(data, this);

        data->prev->next = data->next;
        data->next->prev = data->prev;

        data->list = NULL;

        size_--;

        Ownership<T>::release(element);
    }

private:
    static inline T* container_of_(ListNode::ListNodeData* data) {
        return static_cast<T*>(data->container_of());
    }

    static void check_is_member_(const ListNode::ListNodeData* data, const List* list) {
        if (data->list != list) {
            roc_panic("list element is member of wrong list: expected %p, got %p",
                      (const void*)list, (const void*)data->list);
        }
    }

    void insert_(T& element, T* before) {
        ListNode::ListNodeData* data_new = element.list_node_data();
        check_is_member_(data_new, NULL);

        ListNode::ListNodeData* data_before;
        if (before != NULL) {
            data_before = before->list_node_data();
            check_is_member_(data_before, this);
        } else {
            data_before = &head_;
        }

        data_new->next = data_before;
        data_new->prev = data_before->prev;

        data_before->prev->next = data_new;
        data_before->prev = data_new;

        data_new->list = this;

        size_++;

        Ownership<T>::acquire(element);
    }

    ListNode::ListNodeData head_;
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_H_
