/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/hashmap.h
//! @brief Intrusive hash table.

#ifndef ROC_CORE_HASHMAP_H_
#define ROC_CORE_HASHMAP_H_

#include "roc_core/aligned_storage.h"
#include "roc_core/attributes.h"
#include "roc_core/hashmap_node.h"
#include "roc_core/hashsum.h"
#include "roc_core/iallocator.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Intrusive hash table.
//!
//! Characteristics:
//!  1) Intrusive. Hash table nodes are stored directly in elements. No allocations
//!     are needed to insert a node. Allocator is used only to allocate an array
//!     of buckets.
//!  2) Collision-chaining. Implemented as an array of buckets, where a bucket is
//!     the head of a doubly-linked lists of bucket elements.
//!  3) Controllable allocations. Allocations and deallocations are performed only
//!     when the hash table is explicitly growed. All other operations don't touch
//!     allocator.
//!  4) Zero allocations for small hash tables. A fixed number of buckets can be
//!     embedded directly into hash table object.
//!  5) Incremental rehashing. After hash table growth, rehashing is performed
//!     incrementally when inserting and removing elements. The slower hash table
//!     size growth is, the less overhead rehashing adds to each operation.
//!
//! Incremental rehashing technique is inspired by Go's map implementation, though
//! there are differences. Load factor value is from Java's Hashmap implementation.
//! Prime numbers for sizes are from https://planetmath.org/goodhashtableprimes.
//!
//! @tparam T defines object type, it should inherit HashmapNode and additionally
//! implement three methods:
//!
//! @code
//!   // compute key hash
//!   static core::hashsum_t key_hash(Key key);
//!
//!   // compare two keys for equality
//!   static bool key_equal(Key key1, Key key2);
//!
//!   // get object key
//!   Key key() const;
//! @endcode
//!
//! "Key" can be any type. Hashmap doesn't use it directly. It is never copied and
//! is always accessed via the three methods above. The hash is computed for a key
//! only once when an object is inserted into hash table.
//!
//! @tparam EmbeddedCapacity defines the capacity embedded directly into Hashmap.
//! It is used instead of dynamic memory while the number of elements is smaller
//! than this capacity. The actual object size occupied to provide the requested
//! capacity is implementation defined.
//!
//! @tparam OwnershipPolicy defines ownership policy which is used to acquire an element
//! ownership when it's added to the hashmap and release ownership when it's removed
//! from the hashmap.
template <class T,
          size_t EmbeddedCapacity = 0,
          template <class TT> class OwnershipPolicy = RefCountedOwnership>
class Hashmap : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename OwnershipPolicy<T>::Pointer Pointer;

    //! Initialize empty hashmap.
    Hashmap(IAllocator& allocator)
        : curr_buckets_(NULL)
        , n_curr_buckets_(0)
        , prev_buckets_(NULL)
        , n_prev_buckets_(0)
        , size_(0)
        , rehash_pos_(0)
        , rehash_remain_nodes_(0)
        , allocator_(allocator) {
        if (EmbeddedCapacity != 0) {
            if (!realloc_buckets_(NumEmbeddedBuckets)) {
                roc_panic("hashmap: initialization failed");
            }
        }
    }

    //! Release ownership of all elements.
    ~Hashmap() {
        release_bucket_array_(curr_buckets_, n_curr_buckets_);
        release_bucket_array_(prev_buckets_, n_prev_buckets_);

        dealloc_buckets_();
    }

    //! Get maximum number of elements that can be added to hashmap before
    //! grow() should be called.
    size_t capacity() const {
        return buckets_capacity_(n_curr_buckets_);
    }

    //! Get number of elements added to hashmap.
    size_t size() const {
        return size_;
    }

    //! Check if element belongs to hashmap.
    //!
    //! @note
    //!  - has O(1) complexity
    //!  - doesn't compute key hashes
    bool contains(const T& element) const {
        const HashmapNode::HashmapNodeData* node = element.hashmap_node_data();

        if (member_of_bucket_array_(curr_buckets_, n_curr_buckets_, node)) {
            return true;
        }

        if (member_of_bucket_array_(prev_buckets_, n_prev_buckets_, node)) {
            return true;
        }

        return false;
    }

    //! Find element in the hashmap by key.
    //!
    //! @returns
    //!  Pointer to the element with given key or NULL if it's not found.
    //!
    //! @note
    //!  - has O(1) complexity in average and O(n) in the worst case
    //!  - computes key hash
    //!
    //! @note
    //!  The worst case is achieved when the hash function produces many collisions.
    template <class Key> Pointer find(const Key& key) const {
        const hashsum_t hash = T::key_hash(key);

        return find_node_(hash, key);
    }

    //! Insert element into hashmap.
    //!
    //! @remarks
    //!  - acquires ownership of @p element
    //!
    //! @pre
    //!  - hashmap size() should be smaller than hashmap capacity()
    //!  - @p element should not be member of any hashmap
    //!  - hashmap shouldn't have an element with the same key
    //!
    //! @note
    //!  - has O(1) complexity in average and O(n) in the worst case
    //!  - computes key hash
    //!  - doesn't make allocations or deallocations
    //!  - proceedes lazy rehashing
    //!
    //! @note
    //!  Insertion speed is higher when insert to remove ratio is close to one or lower,
    //!  and slows down when it becomes higher than one. The slow down is caused by
    //!  the incremental rehashing algorithm.
    void insert(T& element) {
        if (size_ >= buckets_capacity_(n_curr_buckets_)) {
            roc_panic(
                "hashmap: attempt to insert into full hashmap before calling grow()");
        }

        HashmapNode::HashmapNodeData* node = element.hashmap_node_data();

        if (node->bucket != NULL) {
            roc_panic(
                "hashmap:"
                " attempt to insert an element which is already a member of %s hashmap",
                contains(element) ? "this" : "another");
        }

        const hashsum_t hash = T::key_hash(element.key());

        if (find_node_(hash, element.key())) {
            roc_panic("hashmap: attempt to insert an element with duplicate key");
        }

        Bucket& bucket = select_bucket_(hash);

        node->hash = hash;
        insert_node_(bucket, node);

        proceed_rehash_(true);

        OwnershipPolicy<T>::acquire(element);
    }

    //! Remove element from hashmap.
    //!
    //! @remarks
    //!  - releases ownership of @p element
    //!
    //! @pre
    //!  @p element should be member of this hashmap.
    //!
    //! @note
    //!  - has O(1) complexity
    //!  - doesn't compute key hash
    //!  - doesn't make allocations or deallocations
    //!  - proceedes lazy rehashing
    void remove(T& element) {
        HashmapNode::HashmapNodeData* node = element.hashmap_node_data();

        if (!contains(element)) {
            roc_panic("hashmap:"
                      " attempt to remove an element which is not a member of %s hashmap",
                      node->bucket == NULL ? "any" : "this");
        }

        remove_node_(node);

        proceed_rehash_(false);

        OwnershipPolicy<T>::release(element);
    }

    //! Grow hashtable capacity.
    //!
    //! @remarks
    //!  Check if hash table is full (size is equal to capacity), and if so, increase
    //!  hash table capacity and initiate incremental rehashing. Rehashing will be
    //!  performed during subsequent insertions and removals.
    //!
    //! @returns
    //!  - true if no growth needed or growth succeeded
    //!  - false if allocation failed
    //!
    //! @note
    //!  - has O(1) complexity
    //!  - doesn't computes key hashes
    //!  - makes allocations and deallocations
    //!  - doesn't proceed lazy rehashing
    ROC_ATTR_NODISCARD bool grow() {
        const size_t cap = buckets_capacity_(n_curr_buckets_);
        roc_panic_if_not(size_ <= cap);

        if (size_ == cap) {
            size_t n_buckets = n_curr_buckets_;
            do {
                n_buckets = get_prime_larger_than_(n_buckets);
            } while (size_ >= buckets_capacity_(n_buckets));

            if (!realloc_buckets_(n_buckets)) {
                return false;
            }

            const size_t new_cap = buckets_capacity_(n_curr_buckets_);
            roc_panic_if_not(size_ < new_cap);
        }

        return true;
    }

private:
    enum {
        // rehash happens when n_elements >= n_buckets * LoafFactorNum / LoafFactorDen
        LoafFactorNum = 75,
        LoafFactorDen = 100,

        // how much buckets are embeded directly into Hashmap object
        NumEmbeddedBuckets =
            ((int)EmbeddedCapacity * LoafFactorDen + LoafFactorNum - 1) / LoafFactorNum
    };

    struct Bucket {
        HashmapNode::HashmapNodeData* head;
    };

    bool realloc_buckets_(size_t n_buckets) {
        roc_panic_if_not(n_buckets > 0);

        roc_panic_if_not(rehash_pos_ == 0);
        roc_panic_if_not(rehash_remain_nodes_ == 0);

        Bucket* buckets;
        if (n_buckets <= NumEmbeddedBuckets
            && curr_buckets_ != (Bucket*)embedded_buckets_.memory()) {
            buckets = (Bucket*)embedded_buckets_.memory();
        } else {
            buckets = (Bucket*)allocator_.allocate(n_buckets * sizeof(Bucket));
            if (buckets == NULL) {
                return false;
            }
        }

        memset(buckets, 0, n_buckets * sizeof(Bucket));

        if (prev_buckets_ && prev_buckets_ != (Bucket*)embedded_buckets_.memory()) {
            allocator_.deallocate(prev_buckets_);
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

    void dealloc_buckets_() {
        if (curr_buckets_ && curr_buckets_ != (Bucket*)embedded_buckets_.memory()) {
            allocator_.deallocate(curr_buckets_);
        }

        if (prev_buckets_ && prev_buckets_ != (Bucket*)embedded_buckets_.memory()) {
            allocator_.deallocate(prev_buckets_);
        }
    }

    bool member_of_bucket_array_(Bucket* buckets,
                                 size_t n_buckets,
                                 const HashmapNode::HashmapNodeData* node) const {
        if (n_buckets == 0) {
            return false;
        }

        Bucket* node_bucket = (Bucket*)node->bucket;

        return node_bucket >= buckets && node_bucket < buckets + n_buckets;
    }

    void release_bucket_array_(Bucket* buckets, size_t n_buckets) {
        if (n_buckets == 0) {
            return;
        }

        for (size_t n = 0; n < n_buckets; n++) {
            HashmapNode::HashmapNodeData* node = buckets[n].head;

            while (node) {
                T* elem = static_cast<T*>(node->container_of());

                node->bucket = NULL;

                node = node->next;
                if (node == buckets[n].head) {
                    node = NULL;
                }

                OwnershipPolicy<T>::release(*elem);
            }
        }
    }

    template <class Key> T* find_node_(hashsum_t hash, const Key& key) const {
        if (n_curr_buckets_ != 0) {
            T* elem = find_in_bucket_(curr_buckets_[hash % n_curr_buckets_], hash, key);
            if (elem) {
                return elem;
            }
        }

        if (n_prev_buckets_ != 0) {
            T* elem = find_in_bucket_(prev_buckets_[hash % n_prev_buckets_], hash, key);
            if (elem) {
                return elem;
            }
        }

        return NULL;
    }

    template <class Key>
    T* find_in_bucket_(const Bucket& bucket, hashsum_t hash, const Key& key) const {
        HashmapNode::HashmapNodeData* node = bucket.head;

        if (node != NULL) {
            do {
                if (node->hash == hash) {
                    T* elem = static_cast<T*>(node->container_of());

                    if (T::key_equal(elem->key(), key)) {
                        return elem;
                    }
                }

                node = node->next;
            } while (node != bucket.head);
        }

        return NULL;
    }

    size_t buckets_capacity_(size_t n_buckets) const {
        return n_buckets * LoafFactorNum / LoafFactorDen;
    }

    Bucket& select_bucket_(hashsum_t hash) const {
        roc_panic_if(n_curr_buckets_ == 0);

        return curr_buckets_[hash % n_curr_buckets_];
    }

    void insert_node_(Bucket& bucket, HashmapNode::HashmapNodeData* node) {
        if (HashmapNode::HashmapNodeData* head = bucket.head) {
            node->next = head;
            node->prev = head->prev;

            head->prev->next = node;
            head->prev = node;
        } else {
            bucket.head = node;

            node->next = node;
            node->prev = node;
        }

        node->bucket = (void*)&bucket;

        size_++;
    }

    void remove_node_(HashmapNode::HashmapNodeData* node) {
        Bucket& bucket = *(Bucket*)node->bucket;

        if (bucket.head == node) {
            if (node->next != node) {
                bucket.head = node->next;
            } else {
                bucket.head = NULL;
            }
        }

        node->prev->next = node->next;
        node->next->prev = node->prev;

        if (member_of_bucket_array_(prev_buckets_, n_prev_buckets_, node)) {
            roc_panic_if_not(rehash_remain_nodes_ > 0);
            rehash_remain_nodes_--;
        }

        node->bucket = NULL;

        size_--;
    }

    void proceed_rehash_(bool in_insert) {
        if (rehash_remain_nodes_ == 0) {
            return;
        }

        size_t num_migrations = 1;

        if (in_insert) {
            const size_t inserts_until_rehash =
                buckets_capacity_(n_curr_buckets_) - size_;

            if (inserts_until_rehash == 0) {
                // migrate all remaining nodes
                num_migrations = rehash_remain_nodes_;
            } else {
                // migrate as much nodes per insert as needed to finish until next rehash
                num_migrations = (rehash_remain_nodes_ + inserts_until_rehash - 1)
                    / inserts_until_rehash;
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

    void migrate_node_(HashmapNode::HashmapNodeData* node) {
        remove_node_(node);

        Bucket& bucket = select_bucket_(node->hash);

        insert_node_(bucket, node);
    }

    size_t get_prime_larger_than_(size_t min) {
        static const size_t primes[] = {
            53,        97,        193,       389,       769,       1543,     3079,
            6151,      12289,     24593,     49157,     98317,     196613,   393241,
            786433,    1572869,   3145739,   6291469,   12582917,  25165843, 50331653,
            100663319, 201326611, 402653189, 805306457, 1610612741
        };

        for (size_t n = 0; n < ROC_ARRAY_SIZE(primes); n++) {
            if (primes[n] > min) {
                return primes[n];
            }
        }

        roc_panic_if(min * 3 < min);
        return min * 3;
    }

    Bucket* curr_buckets_;
    size_t n_curr_buckets_;

    Bucket* prev_buckets_;
    size_t n_prev_buckets_;

    size_t size_;

    size_t rehash_pos_;
    size_t rehash_remain_nodes_;

    IAllocator& allocator_;

    AlignedStorage<NumEmbeddedBuckets * sizeof(Bucket)> embedded_buckets_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASHMAP_H_
