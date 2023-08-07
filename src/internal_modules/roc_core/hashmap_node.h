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

//! Base class for hashmap element.
//! @remarks
//!  Object should inherit this class to be able to be a member of Hashmap.
class HashmapNode : public NonCopyable<HashmapNode> {
public:
    //! Hashmap node data.
    struct HashmapNodeData {
        //! Previous node in bucket.
        HashmapNodeData* bucket_prev;

        //! Next node in bucket.
        HashmapNodeData* bucket_next;

        //! Previous node in list of all nodes.
        HashmapNodeData* all_prev;

        //! Next node in in list of all nodes.
        HashmapNodeData* all_next;

        //! Cached node hash.
        hashsum_t hash;

        //! The bucket this node belongs to.
        //! @remarks
        //!  NULL if node is not member of any hashmap.
        void* bucket;

        HashmapNodeData()
            : bucket_prev(NULL)
            , bucket_next(NULL)
            , all_prev(NULL)
            , all_next(NULL)
            , hash(0)
            , bucket(NULL) {
        }

        //! Get HashmapNode object that contains this HashmapData object.
        HashmapNode* container_of() {
            return ROC_CONTAINER_OF(this, HashmapNode, hashmap_data_);
        }
    };

    ~HashmapNode() {
        if (hashmap_data_.bucket != NULL) {
            roc_panic("hashmap node:"
                      " can't call destructor for an element that is still in hashmap");
        }
    }

    //! Get hashmap node data.
    HashmapNodeData* hashmap_node_data() const {
        return &hashmap_data_;
    }

private:
    mutable HashmapNodeData hashmap_data_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASHMAP_NODE_H_
