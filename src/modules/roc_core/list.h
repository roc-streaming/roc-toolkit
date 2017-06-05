/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
#include "roc_core/shared_ptr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive doubly-linked list.
//!
//! @tparam T defines object type which should inherit ListNode.
//! @tparam Ownership may be RefCntOwnership, NoOwnership or something else;
//! it can be used to automatically acquire and release ownership of
//! object when it is added or removed from list.
template <class T, template <class TT> class Ownership = RefCntOwnership>
class List : public NonCopyable<> {
    typedef typename Ownership<T>::SafePtr Ptr;

public:
    //! Initialize empty list.
    List()
        : size_(0) {
        head_.prev = &head_;
        head_.next = &head_;
        head_.list = this;
    }

    //! Release ownership of containing objects.
    ~List() {
        ListNode::Node* next_node;

        for (ListNode::Node* node = head_.next; node != &head_; node = next_node) {
            roc_panic_if(node == NULL);
            roc_panic_if(node->list != this);

            next_node = node->next;
            node->list = NULL;

            Ownership<T>::release(*static_cast<T*>(node->container()));
        }

        head_.list = NULL;
    }

    //! Get number of elements in list.
    size_t size() const {
        return size_;
    }

    //! Get first list element.
    //!
    //! @returns first element or NULL if list is empty.
    Ptr front() const {
        if (size_ == 0) {
            return NULL;
        }
        return static_cast<T*>(head_.next->container());
    }

    //! Get last list element.
    //!
    //! @returns last element or NULL if list is empty.
    Ptr back() const {
        if (size_ == 0) {
            return NULL;
        }
        return static_cast<T*>(head_.prev->container());
    }

    //! Get list element next to given one.
    //!
    //! @returns
    //!  list element following @p element if @p element is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this list.
    Ptr next(T& element) const {
        roc_panic_if((const void*)&element == NULL);

        ListNode::Node* node = element.listnode();
        check_is_member_(node, this);

        if (node->next == &head_) {
            return NULL;
        }
        return static_cast<T*>(node->next->container());
    }

    //! Append element to list.
    //!
    //! @remarks
    //!  - Appends @p element to list.
    //!  - Acquires ownership of @p element.
    //!
    //! @pre
    //!  @p element should not be member of any list.
    void append(T& element) {
        insert(element, NULL);
    }

    //! Insert element into list.
    //!
    //! @remarks
    //!  - Inserts @p element before @p before if it's not NULL, or to
    //!    the end of list otherwise.
    //!  - Acquires ownership of @p element.
    //!
    //! @pre
    //!  @p element should not be member of any list.
    //!  @p before should be member of this list or NULL.
    void insert(T& element, T* before) {
        roc_panic_if((const void*)&element == NULL);

        ListNode::Node* node_new = element.listnode();
        check_is_member_(node_new, NULL);

        ListNode::Node* node_before;

        if (before != NULL) {
            node_before = before->listnode();
            check_is_member_(node_before, this);
        } else {
            node_before = &head_;
        }

        node_new->next = node_before;
        node_new->prev = node_before->prev;

        node_before->prev->next = node_new;
        node_before->prev = node_new;

        node_new->list = this;

        size_++;

        Ownership<T>::acquire(element);
    }

    //! Remove element from list.
    //!
    //! @remarks
    //!  - Removes @p element from list.
    //!  - Releases ownership of @p element.
    //!
    //! @pre
    //!  @p element should be member of this list.
    void remove(T& element) {
        roc_panic_if((const void*)&element == NULL);

        ListNode::Node* node = element.listnode();
        check_is_member_(node, this);

        node->prev->next = node->next;
        node->next->prev = node->prev;

        node->list = NULL;

        size_--;

        Ownership<T>::release(element);
    }

private:
    static void check_is_member_(const ListNode::Node* node, const List* list) {
        if (node->list != list) {
            roc_panic("list element is member of wrong list (expected %p, got %p)",
                      (const void*)list, (const void*)node->list);
        }
    }

    ListNode::Node head_;
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LIST_H_
