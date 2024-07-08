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

HashmapImpl::HashmapImpl(void* preallocated_data,
                         size_t num_preallocated_buckets,
                         IArena& arena)
    : preallocated_data_(preallocated_data)
    , num_preallocated_buckets_(num_preallocated_buckets)
    , curr_buckets_(NULL)
    , n_curr_buckets_(0)
    , prev_buckets_(NULL)
    , n_prev_buckets_(0)
    , size_(0)
    , rehash_pos_(0)
    , rehash_remain_nodes_(0)
    , arena_(arena) {
    all_head_.all_prev = &all_head_;
    all_head_.all_next = &all_head_;
}

HashmapImpl::~HashmapImpl() {
    if (size_ != 0) {
        roc_panic("hashmap: hashmap isn't empty on destruct");
    }
    dealloc_buckets_();
}

size_t HashmapImpl::capacity() const {
    return buckets_capacity_(n_curr_buckets_);
}

size_t HashmapImpl::size() const {
    return size_;
}

bool HashmapImpl::contains(const HashmapData* node) const {
    if (member_of_bucket_array_(curr_buckets_, n_curr_buckets_, node)) {
        return true;
    }

    if (member_of_bucket_array_(prev_buckets_, n_prev_buckets_, node)) {
        return true;
    }

    return false;
}

HashmapData* HashmapImpl::find_node(hashsum_t hash,
                                    const void* key,
                                    key_equals_callback key_equals) const {
    if (n_curr_buckets_ != 0) {
        HashmapData* elem =
            find_in_bucket_(curr_buckets_[hash % n_curr_buckets_], hash, key, key_equals);
        if (elem) {
            return elem;
        }
    }

    if (n_prev_buckets_ != 0) {
        HashmapData* elem =
            find_in_bucket_(prev_buckets_[hash % n_prev_buckets_], hash, key, key_equals);
        if (elem) {
            return elem;
        }
    }

    return NULL;
}

HashmapData* HashmapImpl::front() const {
    if (size() == 0) {
        return NULL;
    }
    return all_head_.all_next;
}

HashmapData* HashmapImpl::back() const {
    if (size() == 0) {
        return NULL;
    }
    return all_head_.all_prev;
}

HashmapData* HashmapImpl::nextof(HashmapData* node) const {
    if (!contains(node)) {
        roc_panic("hashmap:"
                  " attempt to use an element which is not a member of %s hashmap",
                  node->bucket == NULL ? "any" : "this");
    }

    if (node->all_next == &all_head_) {
        return NULL;
    }

    return node->all_next;
}

HashmapData* HashmapImpl::prevof(HashmapData* node) const {
    if (!contains(node)) {
        roc_panic("hashmap:"
                  " attempt to use an element which is not a member of %s hashmap",
                  node->bucket == NULL ? "any" : "this");
    }

    if (node->all_prev == &all_head_) {
        return NULL;
    }

    return node->all_prev;
}

bool HashmapImpl::insert(HashmapData* node,
                         hashsum_t hash,
                         const void* key,
                         key_equals_callback key_equals) {
    if (size_ >= buckets_capacity_(n_curr_buckets_)) {
        if (!grow()) {
            return false;
        }
    }

    if (node->bucket != NULL) {
        roc_panic("hashmap:"
                  " attempt to insert an element which is already a member of %s hashmap",
                  contains(node) ? "this" : "another");
    }

    if (find_node(hash, key, key_equals)) {
        roc_panic("hashmap: attempt to insert an element with duplicate key");
    }

    Bucket& bucket = select_bucket_(hash);

    node->hash = hash;
    bucket_insert_(bucket, node);
    all_list_insert_(node);
    size_++;

    proceed_rehash_(true);

    return true;
}

void HashmapImpl::remove(HashmapData* node, bool skip_rehash) {
    if (!contains(node)) {
        roc_panic("hashmap:"
                  " attempt to remove an element which is not a member of %s hashmap",
                  node->bucket == NULL ? "any" : "this");
    }

    bucket_remove_(node);
    all_list_remove_(node);
    size_--;

    if (!skip_rehash) {
        proceed_rehash_(false);
    }
}

bool HashmapImpl::grow() {
    const size_t cap = buckets_capacity_(n_curr_buckets_);
    roc_panic_if_not(size_ <= cap);

    if (size_ == cap) {
        size_t n_buckets = n_curr_buckets_;
        do {
            n_buckets = get_next_bucket_size_(n_buckets);
        } while (size_ >= buckets_capacity_(n_buckets));

        if (!realloc_buckets_(n_buckets)) {
            return false;
        }

        const size_t new_cap = buckets_capacity_(n_curr_buckets_);
        roc_panic_if_not(size_ < new_cap);
    }

    return true;
}

HashmapData* HashmapImpl::find_in_bucket_(const Bucket& bucket,
                                          hashsum_t hash,
                                          const void* key,
                                          key_equals_callback key_equals) const {
    HashmapData* node = bucket.head;

    if (node != NULL) {
        do {
            if (node->hash == hash) {
                if (key_equals(node, key)) {
                    return node;
                }
            }

            node = node->bucket_next;
        } while (node != bucket.head);
    }

    return NULL;
}

size_t HashmapImpl::buckets_capacity_(size_t n_buckets) const {
    return n_buckets * LoadFactorNum / LoadFactorDen;
}

bool HashmapImpl::realloc_buckets_(size_t n_buckets) {
    roc_panic_if_not(n_buckets > 0);

    roc_panic_if_not(rehash_pos_ == 0);
    roc_panic_if_not(rehash_remain_nodes_ == 0);

    Bucket* buckets;
    if (n_buckets <= num_preallocated_buckets_
        && curr_buckets_ != (Bucket*)preallocated_data_) {
        buckets = (Bucket*)preallocated_data_;
    } else {
        buckets = (Bucket*)arena_.allocate(n_buckets * sizeof(Bucket));
        if (buckets == NULL) {
            return false;
        }
    }

    memset(buckets, 0, n_buckets * sizeof(Bucket));

    if (prev_buckets_ && prev_buckets_ != (Bucket*)preallocated_data_) {
        arena_.deallocate(prev_buckets_);
        prev_buckets_ = NULL;
    }

    if (curr_buckets_) {
        prev_buckets_ = curr_buckets_;
        n_prev_buckets_ = n_curr_buckets_;

        rehash_pos_ = 0;
        rehash_remain_nodes_ = size_;
    }

    curr_buckets_ = buckets;
    n_curr_buckets_ = n_buckets;

    return true;
}

void HashmapImpl::dealloc_buckets_() {
    if (curr_buckets_ && curr_buckets_ != (Bucket*)preallocated_data_) {
        arena_.deallocate(curr_buckets_);
    }

    if (prev_buckets_ && prev_buckets_ != (Bucket*)preallocated_data_) {
        arena_.deallocate(prev_buckets_);
    }
}

bool HashmapImpl::member_of_bucket_array_(Bucket* buckets,
                                          size_t n_buckets,
                                          const HashmapData* node) const {
    if (n_buckets == 0) {
        return false;
    }

    Bucket* node_bucket = (Bucket*)node->bucket;

    return node_bucket >= buckets && node_bucket < buckets + n_buckets;
}

HashmapImpl::Bucket& HashmapImpl::select_bucket_(hashsum_t hash) const {
    roc_panic_if(n_curr_buckets_ == 0);

    return curr_buckets_[hash % n_curr_buckets_];
}

void HashmapImpl::bucket_insert_(Bucket& bucket, HashmapData* node) {
    if (HashmapData* head = bucket.head) {
        node->bucket_next = head;
        node->bucket_prev = head->bucket_prev;

        head->bucket_prev->bucket_next = node;
        head->bucket_prev = node;
    } else {
        bucket.head = node;

        node->bucket_next = node;
        node->bucket_prev = node;
    }

    node->bucket = (void*)&bucket;
}

void HashmapImpl::bucket_remove_(HashmapData* node) {
    Bucket& bucket = *(Bucket*)node->bucket;

    if (bucket.head == node) {
        if (node->bucket_next != node) {
            bucket.head = node->bucket_next;
        } else {
            bucket.head = NULL;
        }
    }

    node->bucket_prev->bucket_next = node->bucket_next;
    node->bucket_next->bucket_prev = node->bucket_prev;

    if (member_of_bucket_array_(prev_buckets_, n_prev_buckets_, node)) {
        roc_panic_if_not(rehash_remain_nodes_ > 0);
        rehash_remain_nodes_--;
    }

    node->bucket = NULL;
}

void HashmapImpl::all_list_insert_(HashmapData* node) {
    node->all_next = &all_head_;
    node->all_prev = all_head_.all_prev;

    all_head_.all_prev->all_next = node;
    all_head_.all_prev = node;
}

void HashmapImpl::all_list_remove_(HashmapData* node) {
    node->all_prev->all_next = node->all_next;
    node->all_next->all_prev = node->all_prev;
}

void HashmapImpl::proceed_rehash_(bool in_insert) {
    if (rehash_remain_nodes_ == 0) {
        return;
    }

    size_t num_migrations = 1;

    if (in_insert) {
        const size_t inserts_until_rehash = buckets_capacity_(n_curr_buckets_) - size_;

        if (inserts_until_rehash == 0) {
            // migrate all remaining nodes
            num_migrations = rehash_remain_nodes_;
        } else {
            // migrate as much nodes per insert as needed to finish until next rehash
            num_migrations =
                (rehash_remain_nodes_ + inserts_until_rehash - 1) / inserts_until_rehash;
        }
    }

    for (;;) {
        roc_panic_if_not(rehash_pos_ < n_prev_buckets_);

        Bucket& bucket = prev_buckets_[rehash_pos_];

        if (bucket.head == NULL) {
            rehash_pos_++;

            if (rehash_pos_ == n_prev_buckets_) {
                roc_panic_if_not(rehash_remain_nodes_ == 0);

                rehash_pos_ = 0;
                n_prev_buckets_ = 0;

                return;
            }
            continue;
        }

        if (num_migrations == 0) {
            return;
        }

        migrate_node_(bucket.head);
        --num_migrations;
    }
}

void HashmapImpl::migrate_node_(HashmapData* node) {
    bucket_remove_(node);

    Bucket& bucket = select_bucket_(node->hash);

    bucket_insert_(bucket, node);
}

size_t HashmapImpl::get_next_bucket_size_(size_t current_count) {
    // roughly doubling sequence of prime numbers, used as bucket counts
    static const size_t prime_counts[] = {
        5,    11,   23,    53,    97,    193,   389,    769,    1543,
        3079, 6151, 12289, 24593, 49157, 98317, 196613, 393241, 786433,
    };

    // minimum bucket count when allocating from arena
    const size_t min_arena_count = 23;

    if ((ssize_t)current_count < (ssize_t)num_preallocated_buckets_) {
        // we are allocating from embedded capacity
        // find maximum prime count above current and below capacity
        for (size_t n = 0; n < ROC_ARRAY_SIZE(prime_counts) - 1; n++) {
            if (prime_counts[n] > num_preallocated_buckets_) {
                break;
            }
            if (prime_counts[n] > current_count
                && prime_counts[n + 1] > num_preallocated_buckets_) {
                return prime_counts[n];
            }
        }
    }

    // we are allocating from arena
    // find minimum prime count above current
    for (size_t n = 0; n < ROC_ARRAY_SIZE(prime_counts); n++) {
        if (prime_counts[n] < min_arena_count) {
            // skip small counts when allocating from arena
            continue;
        }
        if (prime_counts[n] > current_count) {
            return prime_counts[n];
        }
    }

    // fallback for unrealistically large counts
    roc_panic_if(current_count * 3 < current_count);
    return current_count * 3;
}

} // namespace core
} // namespace roc
