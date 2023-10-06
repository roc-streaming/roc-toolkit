/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/hashmap_impl.h"

namespace roc {
namespace core {

HashmapImpl::HashmapImpl(void* preallocated_data, size_t preallocated_size)
    : preallocated_data_(preallocated_data)
    , preallocated_size_(preallocated_size)
    , curr_buckets_(NULL)
    , n_curr_buckets_(0)
    , prev_buckets_(NULL)
    , n_prev_buckets_(0)
    , size_(0)
    , rehash_pos_(0)
    , rehash_remain_nodes_(0)
    , arena_(NULL) {
    all_head_.all_prev = &all_head_;
    all_head_.all_next = &all_head_;
}

HashmapImpl::HashmapImpl(void* preallocated_data, size_t preallocated_size, IArena& arena)
    : preallocated_data_(preallocated_data)
    , preallocated_size_(preallocated_size)
    , curr_buckets_(NULL)
    , n_curr_buckets_(0)
    , prev_buckets_(NULL)
    , n_prev_buckets_(0)
    , size_(0)
    , rehash_pos_(0)
    , rehash_remain_nodes_(0)
    , arena_(&arena) {
    all_head_.all_prev = &all_head_;
    all_head_.all_next = &all_head_;
}

HashmapImpl::~HashmapImpl() {
    //    release_bucket_array_(curr_buckets_, n_curr_buckets_);
    //    release_bucket_array_(prev_buckets_, n_prev_buckets_);
    //
    //    dealloc_buckets_();
}

size_t HashmapImpl::capacity() const {
    return buckets_capacity_(n_curr_buckets_);
}

size_t HashmapImpl::size() const {
    return size_;
}

bool HashmapImpl::contains(const HashmapNode::HashmapNodeData* node) const {
    if (member_of_bucket_array_(curr_buckets_, n_curr_buckets_, node)) {
        return true;
    }

    if (member_of_bucket_array_(prev_buckets_, n_prev_buckets_, node)) {
        return true;
    }

    return false;
}

bool HashmapImpl::member_of_bucket_array_(
    Bucket* buckets, size_t n_buckets, const HashmapNode::HashmapNodeData* node) const {
    if (n_buckets == 0) {
        return false;
    }

    Bucket* node_bucket = (Bucket*)node->bucket;

    return node_bucket >= buckets && node_bucket < buckets + n_buckets;
}

} // namespace core
} // namespace roc
