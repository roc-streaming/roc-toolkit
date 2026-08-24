/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/pairing_heap.h
//! @brief Intrusive pairing heap.

#ifndef ROC_CORE_PAIRING_HEAP_H_
#define ROC_CORE_PAIRING_HEAP_H_

#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/pairing_heap_node.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive pairing heap.
//!
//! Does not perform allocations.
//! Provides O(1) size check, membership check, insertion, and removal.
//!
//! @tparam T defines object type, it should inherit PairingHeapNode.
//!
//! @tparam OwnershipPolicy defines ownership policy which is used to acquire an
//! element ownership when it's added to the pairing heap and release ownership when it's
//! removed from the pairing heap.
template <class T, template <class TT> class OwnershipPolicy = RefCountedOwnership>
class PairingHeap : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename OwnershipPolicy<T>::Pointer Pointer;

    //! Initialize empty pairing heap.
    PairingHeap()
        : size_(0) {
        root_.leftmost_child = &root_;
        root_.prev = &root_;
        root_.next = &root_;
        root_.pairing_heap = this;
    }

    //! Parse through pairing heap to release ownership of containing objects.
    ~PairingHeap() {
        if ((root_.leftmost_child != &root_ && root_.leftmost_child != NULL)
            || size_ > 0) {
            ReleasePairingHeapNode(root_.leftmost_child);
        }

        root_.pairing_heap = NULL;
    }

    //! Release ownership of containing objects.
    void ReleasePairingHeapNode(PairingHeapNode::PairingHeapNodeData* data) {
        roc_panic_if(data == NULL);
        check_is_member_(data, this);

        if (data->leftmost_child != NULL) {
            ReleasePairingHeapNode(data->leftmost_child);
        }

        if (data->next != NULL) {
            ReleasePairingHeapNode(data->next);
        }

        data->pairing_heap = NULL;

        OwnershipPolicy<T>::release(*container_of_(data));
    }

    //! Get number of elements in pairing heap.
    size_t size() const {
        return size_;
    }

    //! Check if element belongs to pairing heap.
    bool contains(const T& element) {
        const PairingHeapNode::PairingHeapNodeData* data =
            element.pairing_heap_node_data();
        return (data->pairing_heap == this);
    }

    //! Get first pairing heap element.
    //! @returns
    //!  first element or NULL if pairing heap is empty.
    Pointer top() const {
        if (size_ == 0) {
            return NULL;
        }
        return container_of_(root_.leftmost_child);
    }

    //! Get pairing heap element next to given one.
    //!
    //! @returns
    //!  pairing heap element following @p element if @p element is not
    //!  the last sibling, or NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this pairing heap.
    Pointer next_sibling_of(T& element) const {
        PairingHeapNode::PairingHeapNodeData* data = element.pairing_heap_node_data();
        check_is_member_(data, this);

        if (data->next == NULL) {
            return NULL;
        }
        return container_of_(data->next);
    }

    //! Get pairing heap element previous to given one.
    //!
    //! @returns
    //!  pairing heap element before @p element if @p element has a previous element, or
    //!  NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this pairing heap.
    Pointer prev_sibling_of(T& element) const {
        PairingHeapNode::PairingHeapNodeData* data = element.pairing_heap_node_data();
        check_is_member_(data, this);

        if (data->prev == NULL) {
            return NULL;
        }
        return container_of_(data->prev);
    }

    //! Get pairing heap element child of given one.
    //!
    //! @returns
    //!  pairing heap element child of @p element if @p element has a child element, or
    //!  NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this pairing heap.
    Pointer child_of(T& element) const {
        PairingHeapNode::PairingHeapNodeData* data = element.pairing_heap_node_data();
        check_is_member_(data, this);

        if (data->leftmost_child == NULL) {
            return NULL;
        }
        return container_of_(data->leftmost_child);
    }

    //! Inserts first element to pairing heap.
    //!
    //! @remarks
    //!  - appends @p element to pairing heap
    //!  - acquires ownership of @p element
    //!
    //! @pre
    //!  @p element should not be member of any pairing heap and should be the first
    //!  element in the heap.
    void push(T& element) {
        insert_as_child(element, NULL);
    }

    //! Insert element into pairing heap as a child of an existing element.
    //!
    //! @remarks
    //!  - inserts @p new_child as a child of @p parent element
    //!  - acquires ownership of @p new_child
    //!
    //! @pre
    //!  @p new_child should not be member of any pairing heap.
    //!  @p parent should be member of this pairing heap.
    void push_as_child(T& new_child, T& parent) {
        insert_as_child(new_child, &parent);
    }

    //! Insert element into pairing heap as a parent of an existing element.
    //!
    //! @remarks
    //!  - inserts @p new_parent as parent of @p child element
    //!  - acquires ownership of @p new_parent
    //!
    //! @pre
    //!  @p new_parent should not be member of any pairing heap.
    //!  @p child should be member of this pairing heap.
    void push_as_parent(T& new_parent, T& child) {
        insert_as_parent(new_parent, &child);
    }

    //! Merge two pairing heap elements by parenting first given element to second given
    //! element.
    //!
    //! @returns
    //!  the @p parent_element pairing heap element
    //!
    //! @pre
    //!  @p parent_element and @p child_element should be members of this pairing heap.
    Pointer merge(T& parent_element, T& child_element) const {
        PairingHeapNode::PairingHeapNodeData* parent =
            parent_element.pairing_heap_node_data();
        PairingHeapNode::PairingHeapNodeData* child =
            child_element.pairing_heap_node_data();

        check_is_member_(parent, this);
        check_is_member_(child, this);

        if (child->prev->leftmost_child == child) {
            child->prev->leftmost_child = parent;
            parent->prev = child->prev;
            child->prev = parent;
        } else {
            if (parent->next == child) {
                parent->next = child->next;
                if (child->next != NULL) {
                    child->next->prev = parent;
                }
            } else {
                parent->prev = child->prev;
            }
        }

        if (parent->leftmost_child != NULL) {
            parent->leftmost_child->prev = child;
        }

        child->next = parent->leftmost_child;
        parent->leftmost_child = child;

        return container_of_(parent);
    }

    //! Remove element from pairing heap.
    //!
    //! @remarks
    //!  - removes @p element from pairing heap
    //!  - releases ownership of @p element
    //!
    //! @pre
    //!  @p element should be member of this pairing heap.
    void remove(T& element) {
        PairingHeapNode::PairingHeapNodeData* data = element.pairing_heap_node_data();
        check_is_member_(data, this);

        PairingHeapNode::PairingHeapNodeData* data_child = data->leftmost_child;

        if (data_child != NULL) {
            data_child->prev = data->prev;
            data_child->next = data->next;

            if (data->prev->leftmost_child == data) {
                data->prev->leftmost_child = data_child;
            } else {
                data->prev->next = data_child;
            }
        } else {
            if (data->prev->leftmost_child == data) {
                data->prev->leftmost_child = data->next;
            } else {
                if (data->prev == &root_) {
                    data->prev->next = &root_;
                } else {
                    data->prev->next = data->next;
                }
            }
        }

        data->pairing_heap = NULL;

        size_--;

        OwnershipPolicy<T>::release(element);
    }

private:
    static inline T* container_of_(PairingHeapNode::PairingHeapNodeData* data) {
        return static_cast<T*>(data->container_of());
    }

    static void check_is_member_(const PairingHeapNode::PairingHeapNodeData* data,
                                 const PairingHeap* pairing_heap) {
        if (data->pairing_heap != pairing_heap) {
            roc_panic("pairing heap element is member of wrong pairing heap: expected "
                      "%p, got %p",
                      (const void*)pairing_heap, (const void*)data->pairing_heap);
        }
    }

    void insert_as_child(T& new_child, T* parent) {
        PairingHeapNode::PairingHeapNodeData* data_new =
            new_child.pairing_heap_node_data();
        check_is_member_(data_new, NULL);

        PairingHeapNode::PairingHeapNodeData* data_parent;
        if (parent != NULL) {
            data_parent = parent->pairing_heap_node_data();
            check_is_member_(data_parent, this);

            if (data_parent->leftmost_child != NULL) {
                data_new->next = data_parent->leftmost_child;
                data_parent->leftmost_child->prev = data_new;
            }

        } else {
            data_parent = &root_;
        }

        data_new->prev = data_parent;
        data_parent->leftmost_child = data_new;

        data_new->pairing_heap = this;

        size_++;

        OwnershipPolicy<T>::acquire(new_child);
    }

    void insert_as_parent(T& new_parent, T* child) {
        PairingHeapNode::PairingHeapNodeData* data_new =
            new_parent.pairing_heap_node_data();
        check_is_member_(data_new, NULL);

        PairingHeapNode::PairingHeapNodeData* data_child =
            child->pairing_heap_node_data();

        data_new->prev = data_child->prev;
        data_new->leftmost_child = data_child;

        data_child->prev->leftmost_child = data_new;
        data_child->prev = data_new;

        data_new->pairing_heap = this;

        size_++;

        OwnershipPolicy<T>::acquire(new_parent);
    }

    PairingHeapNode::PairingHeapNodeData root_;
    size_t size_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_PAIRING_HEAP_H_
