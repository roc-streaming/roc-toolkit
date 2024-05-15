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
//! @tparam T defines object type, it must inherit HashmapNode and additionally
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
//!
//! @tparam Node defines base class of hashmap nodes. It is needed if HashmapNode
//! is used with non-default tag.
template <class T,
          size_t EmbeddedCapacity = 0,
          template <class TT> class OwnershipPolicy = RefCountedOwnership,
          class Node = HashmapNode<> >
class Hashmap : public NonCopyable<> {
public:
    //! Pointer type.
    //! @remarks
    //!  either raw or smart pointer depending on the ownership policy.
    typedef typename OwnershipPolicy<T>::Pointer Pointer;

    //! Initialize empty hashmap with arena.
    //! @remarks
    //!  Hashmap capacity may grow using arena.
    explicit Hashmap(IArena& arena)
        : impl_(embedded_buckets_.memory(), NumEmbeddedBuckets, arena) {
    }

    //! Release ownership of all elements.
    ~Hashmap() {
        HashmapData* data = impl_.front();

        while (data != NULL) {
            impl_.remove(data, true);
            T* elem = from_node_data_(data);
            OwnershipPolicy<T>::release(*elem);
            data = impl_.front();
        }
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
    bool contains(const T& elem) const {
        const HashmapData* data = to_node_data_(elem);
        return impl_.contains(data);
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
        HashmapData* data = impl_.find_node(
            hash, (const void*)&key,
            &Hashmap<T, EmbeddedCapacity, OwnershipPolicy, Node>::key_equal_<Key>);
        if (!data) {
            return NULL;
        }
        return from_node_data_(data);
    }

    //! Get first element in hashmap.
    //! Elements are ordered by insertion.
    //! @returns
    //!  first element or NULL if hashmap is empty.
    Pointer front() const {
        HashmapData* data = impl_.front();
        if (!data) {
            return NULL;
        }
        return from_node_data_(data);
    }

    //! Get last element in hashmap.
    //! Elements are ordered by insertion.
    //! @returns
    //!  last element or NULL if hashmap is empty.
    Pointer back() const {
        HashmapData* node = impl_.back();
        if (!node) {
            return NULL;
        }
        return from_node_data_(node);
    }

    //! Get hashmap element next to given one.
    //! Elements are ordered by insertion.
    //!
    //! @returns
    //!  hashmap element following @p elem if @p elem is not
    //!  last, or NULL otherwise.
    //!
    //! @pre
    //!  @p elem should be member of this hashmap.
    Pointer nextof(T& elem) const {
        HashmapData* data = to_node_data_(elem);
        HashmapData* next_data = impl_.nextof(data);
        if (!next_data) {
            return NULL;
        }
        return from_node_data_(next_data);
    }

    //! Get hashmap element previous to given one.
    //! Elements are ordered by insertion.
    //!
    //! @returns
    //!  hashmap element preceding @p elem if @p elem is not
    //!  first, or NULL otherwise.
    //!
    //! @pre
    //!  @p elem should be member of this hashmap.
    Pointer prevof(T& elem) const {
        HashmapData* data = to_node_data_(elem);
        HashmapData* prev_data = impl_.prevof(data);
        if (!prev_data) {
            return NULL;
        }
        return from_node_data_(prev_data);
    }

    //! Insert element into hashmap.
    //!
    //! @remarks
    //!  - acquires ownership of @p elem
    //!
    //! @returns
    //!  false if the allocation failed
    //!
    //! @pre
    //!  - @p elem should not be member of any hashmap
    //!  - hashmap shouldn't have an element with the same key
    //!
    //! @note
    //!  - has O(1) complexity in average and O(n) in the worst case
    //!  - computes key hash
    //!  - doesn't make allocations or deallocations
    //!  - proceeds lazy rehashing
    //!
    //! @note
    //!  Insertion speed is higher when insert to remove ratio is close to one or lower,
    //!  and slows down when it becomes higher than one. The slow down is caused by
    //!  the incremental rehashing algorithm.
    ROC_ATTR_NODISCARD bool insert(T& elem) {
        HashmapData* data = to_node_data_(elem);
        if (!insert_(elem.key(), data)) {
            return false;
        }
        OwnershipPolicy<T>::acquire(elem);
        return true;
    }

    //! Remove element from hashmap.
    //!
    //! @remarks
    //!  - releases ownership of @p elem
    //!
    //! @pre
    //!  @p elem should be member of this hashmap.
    //!
    //! @note
    //!  - has O(1) complexity
    //!  - doesn't compute key hash
    //!  - doesn't make allocations or deallocations
    //!  - proceeds lazy rehashing
    void remove(T& elem) {
        HashmapData* data = to_node_data_(elem);
        impl_.remove(data, false);
        OwnershipPolicy<T>::release(elem);
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
        // how much buckets are embedded directly into Hashmap object
        NumEmbeddedBuckets = ((int)(EmbeddedCapacity == 0        ? 0
                                        : EmbeddedCapacity <= 16 ? 16
                                                                 : EmbeddedCapacity)
                                  * HashmapImpl::LoadFactorDen
                              + HashmapImpl::LoadFactorNum - 1)
            / HashmapImpl::LoadFactorNum * 2
    };

    static HashmapData* to_node_data_(const T& elem) {
        return static_cast<const Node&>(elem).hashmap_data();
    }

    static T* from_node_data_(HashmapData* data) {
        return static_cast<T*>(static_cast<Node*>(Node::hashmap_node(data)));
    }

    template <class Key> static bool key_equal_(HashmapData* node, const void* key) {
        T* elem = from_node_data_(node);
        const Key& key_ref = *(const Key*)key;
        return T::key_equal(elem->key(), key_ref);
    }

    template <class Key> bool insert_(const Key& key, HashmapData* node) {
        const hashsum_t hash = T::key_hash(key);
        return impl_.insert(
            node, hash, (const void*)&key,
            &Hashmap<T, EmbeddedCapacity, OwnershipPolicy, Node>::key_equal_<Key>);
    }

    AlignedStorage<NumEmbeddedBuckets * sizeof(HashmapImpl::Bucket)> embedded_buckets_;

    HashmapImpl impl_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_HASHMAP_H_
