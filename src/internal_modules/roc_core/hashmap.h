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
#include "roc_core/hashmap_impl.h"
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

//! Intrusive hash table.
//!
//! Characteristics:
//!  1) Intrusive. Hash table nodes are stored directly in elements. No allocations
//!     are needed to insert a node. Arena is used only to allocate an array
//!     of buckets.
//!  2) Collision-chaining. Implemented as an array of buckets, where a bucket is
//!     the head of a doubly-linked lists of bucket elements.
//!  3) Controllable allocations. Allocations and deallocations are performed only
//!     when the hash table is explicitly growed. All other operations don't touch
//!     arena.
//!  4) Zero allocations for small hash tables. A fixed number of buckets can be
//!     embedded directly into hash table object.
//!  5) Incremental rehashing. After hash table growth, rehashing is performed
//!     incrementally when inserting and removing elements. The slower hash table
//!     size growth is, the less overhead rehashing adds to each operation.
//!  6) Allows to iterate elements in insertion order. Implements safe iteration with
//!     regards to element insertion and deletion. Elements deleted during iteration
//!     won't be visited. Elements inserted during iteration will be visited.
//!
//! Incremental rehashing technique is inspired by Go's map implementation, though
//! there are differences. Load factor value is taken from it as well.
//! Prime numbers for sizes are from https://planetmath.org/goodhashtableprimes.
//!
//! @tparam T defines object type, it should inherit HashmapNode and additionally
//! implement three methods:
//!
//! @code
//!   // get object key
//!   Key key() const;
//!
//!   // compute key hash
//!   static core::hashsum_t key_hash(Key key);
//!
//!   // compare two keys for equality
//!   static bool key_equal(Key key1, Key key2);
//! @endcode
//!
//! "Key" can be any type. Hashmap doesn't use it directly. It is never stored and
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

    //! Initialize empty hashmap without arena.
    //! @remarks
    //!  Hashmap capacity will be limited to the embedded capacity.
    Hashmap()
        : impl_(
            embedded_buckets_.memory(), embedded_buckets_.size(), NumEmbeddedBuckets) {
    }

    //! Initialize empty hashmap with arena.
    //! @remarks
    //!  Hashmap capacity may grow using arena.
    explicit Hashmap(IArena& arena)
        : impl_(embedded_buckets_.memory(),
                embedded_buckets_.size(),
                NumEmbeddedBuckets,
                arena) {
    }

    //! Release ownership of all elements.
    ~Hashmap() {
        impl_.release_all(&Hashmap<T, EmbeddedCapacity, OwnershipPolicy>::node_release);
    }

    //! Get maximum number of elements that can be added to hashmap before
    //! grow() should be called.
    size_t capacity() const {
        return impl_.capacity();
    }

    //! Get number of elements added to hashmap.
    size_t size() const {
        return impl_.size();
    }

    //! Check if size is zero.
    bool is_empty() const {
        return size() == 0;
    }

    //! Check if element belongs to hashmap.
    //!
    //! @note
    //!  - has O(1) complexity
    //!  - doesn't compute key hashes
    bool contains(const T& element) const {
        const HashmapNode::HashmapNodeData* node = element.hashmap_node_data();
        return impl_.contains(node);
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
        HashmapNode::HashmapNodeData* node = impl_.find_node(
            hash, (void*)&key,
            &Hashmap<T, EmbeddedCapacity, OwnershipPolicy>::key_equals<Key>);
        if (!node) {
            return NULL;
        }
        return container_of_(node);
    }

    //! Get first element in hashmap.
    //! Elements are ordered by insertion.
    //! @returns
    //!  first element or NULL if hashmap is empty.
    Pointer front() const {
        HashmapNode::HashmapNodeData* node = impl_.front();
        if (!node) {
            return NULL;
        }
        return container_of_(node);
    }

    //! Get last element in hashmap.
    //! Elements are ordered by insertion.
    //! @returns
    //!  last element or NULL if hashmap is empty.
    Pointer back() const {
        HashmapNode::HashmapNodeData* node = impl_.back();
        if (!node) {
            return NULL;
        }
        return container_of_(node);
    }

    //! Get hashmap element next to given one.
    //! Elements are ordered by insertion.
    //!
    //! @returns
    //!  hashmap element following @p element if @p element is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p element should be member of this hashmap.
    Pointer nextof(T& element) const {
        HashmapNode::HashmapNodeData* node = element.hashmap_node_data();
        HashmapNode::HashmapNodeData* next_node = impl_.nextof(node);
        if (!next_node) {
            return NULL;
        }
        return container_of_(next_node);
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
        HashmapNode::HashmapNodeData* node = element.hashmap_node_data();
        insert_(element.key(), node);
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
        impl_.remove(node);
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
        return impl_.grow();
    }

private:
    enum {
        // how much buckets are embeded directly into Hashmap object
        NumEmbeddedBuckets = ((int)(EmbeddedCapacity == 0        ? 0
                                        : EmbeddedCapacity <= 16 ? 16
                                                                 : EmbeddedCapacity)
                                  * HashmapImpl::LoadFactorDen
                              + HashmapImpl::LoadFactorNum - 1)
            / HashmapImpl::LoadFactorNum * 2
    };

    static T* container_of_(HashmapNode::HashmapNodeData* data) {
        return static_cast<T*>(data->container_of());
    }

    template <class Key>
    static bool key_equals(HashmapNode::HashmapNodeData* node, void* key) {
        T* elem = container_of_(node);
        const Key& key_ref = *(Key*)key;
        return T::key_equal(elem->key(), key_ref);
    }

    static void node_release(HashmapNode::HashmapNodeData* node) {
        T* elem = container_of_(node);
        OwnershipPolicy<T>::release(*elem);
    }

    template <class Key>
    void insert_(const Key& key, HashmapNode::HashmapNodeData* node) {
        const hashsum_t hash = T::key_hash(key);
        impl_.insert(node, hash, (void*)&key,
                     &Hashmap<T, EmbeddedCapacity, OwnershipPolicy>::key_equals<Key>);
    }

    HashmapImpl impl_;

    AlignedStorage<NumEmbeddedBuckets * sizeof(HashmapImpl::Bucket)> embedded_buckets_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASHMAP_H_
