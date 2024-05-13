/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/hashmap_node.h
//! @brief Hashmap node.

#ifndef ROC_CORE_HASHMAP_NODE_H_
#define ROC_CORE_HASHMAP_NODE_H_

#include "roc_core/hashsum.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Hashmap node internal data.
struct HashmapData {
    //! Previous node in bucket.
    HashmapData* bucket_prev;

    //! Next node in bucket.
    HashmapData* bucket_next;

    //! Previous node in list of all nodes.
    HashmapData* all_prev;

    //! Next node in in list of all nodes.
    HashmapData* all_next;

    //! Cached node hash.
    hashsum_t hash;

    //! The bucket this node belongs to.
    //! @remarks
    //!  NULL if node is not member of any hashmap.
    void* bucket;

    HashmapData()
        : bucket_prev(NULL)
        , bucket_next(NULL)
        , all_prev(NULL)
        , all_next(NULL)
        , hash(0)
        , bucket(NULL) {
    }
};

//! Base class for Hashmap element.
//! @remarks
//!  Object should inherit this class to be able to be a member of Hashmap.
//!  Tag allows to inherit multiple copies of ListNode and include same
//!  object into multiple lists.
template <class Tag = void> class HashmapNode : public NonCopyable<HashmapNode<Tag> > {
public:
    ~HashmapNode() {
        if (hashmap_data_.bucket != NULL) {
            roc_panic(
                "hashmap node: attempt to destroy node while it's still in hashmap");
        }
    }

    //! Get pointer to parent node from pointer to internal data.
    static HashmapNode* hashmap_node(HashmapData* data) {
        return ROC_CONTAINER_OF(data, HashmapNode, hashmap_data_);
    }

    //! Get pointer to internal data.
    HashmapData* hashmap_data() const {
        return &hashmap_data_;
    }

private:
    mutable HashmapData hashmap_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASHMAP_NODE_H_
