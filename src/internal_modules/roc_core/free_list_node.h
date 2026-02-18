/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/free_list_node.h
//! @brief Free list node.

#ifndef ROC_CORE_FREE_LIST_NODE_H_
#define ROC_CORE_FREE_LIST_NODE_H_

#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Free list node internal data.
struct FreeListData {
    //! Next free list element.
    FreeListData* next = nullptr;

    //! Reference counter for free list.
    uint32_t refs = 0;
};

//! Base class for Free List element.
//! @remarks
//!  Object should inherit this class to be able to be a member of List.
//!  Tag allows to inherit multiple copies of FreeListNode and include same
//!  object into multiple lists.
template <class Tag = void> class FreeListNode : public NonCopyable<FreeListNode<Tag> > {
public:
    ~FreeListNode() {
    }

    //! Get pointer to parent node from pointer to internal data.
    static FreeListNode* list_node(FreeListData* data) {
        return ROC_CONTAINER_OF(data, FreeListNode, free_list_data_);
    }

    //! Get pointer to internal data.
    FreeListData* list_data() const {
        return &free_list_data_;
    }

private:
    mutable FreeListData free_list_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_FREE_LIST_NODE_H_
