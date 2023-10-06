/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/hashmap_impl.h
//! @brief Intrusive hash table implementation file.

#ifndef ROC_CORE_HASHMAP_IMPL_H_
#define ROC_CORE_HASHMAP_IMPL_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/attributes.h"
#include "roc_core/hashmap_node.h"
#include "roc_core/hashsum.h"
#include "roc_core/iarena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

class HashmapImpl {
public:
    struct Bucket {
        HashmapNode::HashmapNodeData* head;
    };

    HashmapImpl(void* preallocated_data, size_t preallocated_size);

    explicit HashmapImpl(void* preallocated_data,
                         size_t preallocated_size,
                         IArena& arena);

    ~HashmapImpl();

    size_t capacity() const;
    size_t size() const;
    bool contains(const HashmapNode::HashmapNodeData* node) const;

private:
    bool member_of_bucket_array_(Bucket* buckets,
                                 size_t n_buckets,
                                 const HashmapNode::HashmapNodeData* node) const;

    void* preallocated_data_;
    size_t preallocated_size_;

    Bucket* curr_buckets_;
    size_t n_curr_buckets_;

    Bucket* prev_buckets_;
    size_t n_prev_buckets_;

    size_t size_;

    size_t rehash_pos_;
    size_t rehash_remain_nodes_;

    // head of list of all nodes
    HashmapNode::HashmapNodeData all_head_;

    IArena* arena_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASHMAP_IMPL_H_
