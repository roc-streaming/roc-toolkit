/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/slab_pool.h
//! @brief Slab pool.

#ifndef ROC_CORE_SLAB_POOL_H_
#define ROC_CORE_SLAB_POOL_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/helpers.h"
#include "roc_core/ipool.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/mutex.h"

namespace roc {
namespace core {

//! Slab pool.
//!
//! @tparam Size defines number of elements in pool.
//! @tparam T defines object type in memory.
template <size_t Size, class T> class SlabPool : public IPool<T>, public NonCopyable<> {
public:
    //! Initialization.
    SlabPool() {
        for (size_t i = 0; i < Size; ++i) {
            ListNode* node = new (elements_[i].u_node.mem()) ListNode();
            free_nodes_.append(*node);
        }
    }

    ~SlabPool() {
        if (free_nodes_.size() != Size) {
            roc_panic("memory leak in slab pool: %u leaked elements",
                      (unsigned)(Size - free_nodes_.size()));
        }
    }

    //! Allocate new object.
    virtual void* allocate() {
        ListNode* node;
        {
            Mutex::Lock lock(mutex_);
            node = free_nodes_.back();
            if (node != NULL) {
                free_nodes_.remove(*node);
            }
        }
        if (node != NULL) {
            Element* elem = ROC_CONTAINER_OF(node, Element, u_node);
            node->~ListNode();
            return elem->u_data.mem();
        } else {
            return NULL;
        }
    }

    //! Destroy previously allocated object and memory.
    virtual void deallocate(void* memory) {
        Element* elem = container_of_(memory);
        ListNode* node = new (elem->u_node.mem()) ListNode();
        {
            Mutex::Lock lock(mutex_);
            free_nodes_.append(*node);
        }
    }

    //! Check if this object belongs to this pool.
    virtual void check(T& object) {
        char* elem = (char*)container_of_(&object);
        if (elem < (char*)&elements_[0] || elem >= (char*)&elements_[Size]) {
            roc_panic("object doesn't belong to this pool");
        }
        if ((unsigned long)(elem - (char*)elements_) % sizeof(Element) != 0) {
            roc_panic("object address is misaligned inside pool");
        }
    }

    //! Number of free object available in pool.
    size_t available() {
        Mutex::Lock lock(mutex_);
        return free_nodes_.size();
    }

private:
    union Element {
        AlignedStorage<T> u_data;
        AlignedStorage<ListNode> u_node;
    };

    Element* container_of_(void* memory) {
        roc_panic_if(memory == NULL);
        return ROC_CONTAINER_OF(&AlignedStorage<T>::container_of(*(T*)memory), Element,
                                u_data);
    }

    Element elements_[Size];

    List<ListNode, NoOwnership> free_nodes_;
    Mutex mutex_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_SLAB_POOL_H_
