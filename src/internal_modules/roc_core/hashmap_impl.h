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

//! Intrusive hash table internal implementation.
class HashmapImpl {
public:
    enum {
        // rehash happens when n_elements >= n_buckets * LoadFactorNum / LoadFactorDen
        LoadFactorNum = 13,
        LoadFactorDen = 2,
    };

    //! Bucket container.
    struct Bucket {
        //! Pointer to head node.
        HashmapNode::HashmapNodeData* head;
    };

    //! Callback function pointer type for key equality check.
    typedef bool (*key_equals_callback)(HashmapNode::HashmapNodeData* node, void* key);

    //! Callback function pointer type for releasing nodes.
    typedef void (*node_release_callback)(HashmapNode::HashmapNodeData* node);

    //! Initialize empty hashmap without arena.
    HashmapImpl(void* preallocated_data, size_t num_embedded_buckets);

    //! Initialize empty hashmap with arena.
    explicit HashmapImpl(void* preallocated_data,
                         size_t num_embedded_buckets,
                         IArena& arena);

    ~HashmapImpl();

    //! Get maximum number of nodes that can be added to hashmap before
    //! grow() should be called.
    size_t capacity() const;

    //! Get number of nodes added to hashmap.
    size_t size() const;

    //! Check if node belongs to hashmap.
    bool contains(const HashmapNode::HashmapNodeData* node) const;

    //! Find node in the hashmap.
    HashmapNode::HashmapNodeData*
    find_node(hashsum_t hash, void* key, key_equals_callback callback) const;

    //! Get first node in hashmap.
    HashmapNode::HashmapNodeData* front() const;

    //! Get last node in hashmap.
    HashmapNode::HashmapNodeData* back() const;

    //! Get hashmap node next to given one.
    HashmapNode::HashmapNodeData* nextof(HashmapNode::HashmapNodeData* node) const;

    //! Insert node into hashmap.
    void insert(HashmapNode::HashmapNodeData* node,
                hashsum_t hash,
                void* key,
                key_equals_callback callback);

    //! Remove node from hashmap.
    void remove(HashmapNode::HashmapNodeData* node);

    //! Grow hashtable capacity.
    ROC_ATTR_NODISCARD bool grow();

private:
    HashmapNode::HashmapNodeData* find_in_bucket_(const Bucket& bucket,
                                                  hashsum_t hash,
                                                  void* key,
                                                  key_equals_callback callback) const;

    size_t buckets_capacity_(size_t n_buckets) const;

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

    void release_bucket_array_(Bucket* buckets,
                               size_t n_buckets,
                               node_release_callback callback);

    void* preallocated_data_;
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
