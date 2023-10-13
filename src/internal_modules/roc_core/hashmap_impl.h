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
    enum {
        // rehash happens when n_elements >= n_buckets * LoadFactorNum / LoadFactorDen
        LoadFactorNum = 13,
        LoadFactorDen = 2,
    };

    struct Bucket {
        HashmapNode::HashmapNodeData* head;
    };

    HashmapImpl(void* preallocated_data,
                size_t preallocated_size,
                size_t num_embedded_buckets);

    explicit HashmapImpl(void* preallocated_data,
                         size_t preallocated_size,
                         size_t num_embedded_buckets,
                         IArena& arena);

    ~HashmapImpl();

    size_t capacity() const;
    size_t size() const;
    bool contains(const HashmapNode::HashmapNodeData* node) const;

    size_t buckets_capacity_(size_t n_buckets) const;

    HashmapNode::HashmapNodeData*
    find_node_(hashsum_t hash,
               void* key,
               bool (*key_equal)(HashmapNode::HashmapNodeData* node, void* key)) const;
    HashmapNode::HashmapNodeData* find_in_bucket_(
        const Bucket& bucket,
        hashsum_t hash,
        void* key,
        bool (*key_equal)(HashmapNode::HashmapNodeData* node, void* key)) const;

    HashmapNode::HashmapNodeData* front() const;
    HashmapNode::HashmapNodeData* back() const;
    HashmapNode::HashmapNodeData* nextof(HashmapNode::HashmapNodeData* node) const;

    void insert(HashmapNode::HashmapNodeData* node,
                hashsum_t hash,
                void* key,
                bool (*key_equal)(HashmapNode::HashmapNodeData* node, void* key));
    void remove(HashmapNode::HashmapNodeData* node);

    ROC_ATTR_NODISCARD bool grow();

    void release_all(void (*release_callback)(HashmapNode::HashmapNodeData* node));

private:
    bool realloc_buckets_(size_t n_buckets);
    void dealloc_buckets_();

    bool member_of_bucket_array_(Bucket* buckets,
                                 size_t n_buckets,
                                 const HashmapNode::HashmapNodeData* node) const;

    Bucket& select_bucket_(hashsum_t hash) const;
    void bucket_insert_(Bucket& bucket, HashmapNode::HashmapNodeData* node);
    void bucket_remove_(HashmapNode::HashmapNodeData* node);
    void all_list_insert_(HashmapNode::HashmapNodeData* node);
    void all_list_remove_(HashmapNode::HashmapNodeData* node);
    void proceed_rehash_(bool in_insert);
    void migrate_node_(HashmapNode::HashmapNodeData* node);
    size_t get_next_bucket_size_(size_t current_count);

    void
    release_bucket_array_(Bucket* buckets,
                          size_t n_buckets,
                          void (*release_callback)(HashmapNode::HashmapNodeData* node));

    void* preallocated_data_;
    size_t preallocated_size_;
    size_t num_embedded_buckets_;

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
