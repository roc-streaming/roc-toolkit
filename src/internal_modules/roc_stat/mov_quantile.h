/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_stat/mov_quantile.h
//! @brief Rolling window quantile.

#ifndef ROC_STAT_MOV_QUANTILE_H_
#define ROC_STAT_MOV_QUANTILE_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/panic.h"

namespace roc {
namespace stat {

//! Rolling window quantile.
//!
//! Efficiently implements moving quantile using partition heap based on approach
//! described in https://aakinshin.net/posts/partitioning-heaps-quantile-estimator/.
//! It follows the quantile estimator strategy mentioned in the document.
//!
//! @tparam T defines a sample type.
template <typename T> class MovQuantile {
public:
    //! Initialize
    MovQuantile(core::IArena& arena, const size_t win_len, const double quantile)
        : win_len_(win_len)
        , quantile_(quantile)
        , old_heap_root_index_(0)
        , heap_root_(0)
        , heap_size_(0)
        , max_heap_index_(0)
        , min_heap_index_(0)
        , elem_index_(0)
        , heap_(arena)
        , elem_index_2_heap_index_(arena)
        , heap_index_2_elem_index_(arena)
        , valid_(false) {
        if (win_len == 0) {
            roc_panic("mov quantile: window length must be greater than 0");
        }
        if (quantile < 0 || quantile > 1) {
            roc_panic("mov quantile: quantile should be between 0 and 1");
        }

        if (!heap_.resize(win_len)) {
            return;
        }
        if (!elem_index_2_heap_index_.resize(win_len)) {
            return;
        }
        if (!heap_index_2_elem_index_.resize(win_len)) {
            return;
        }

        heap_root_ = size_t(quantile * double(win_len - 1));
        max_heap_index_ = heap_root_;
        min_heap_index_ = heap_root_;

        valid_ = true;
    }

    //! Check that initial allocation succeeded.
    bool is_valid() {
        return valid_;
    }

    //! Check if the window is fully filled.
    size_t is_full() const {
        return heap_size_ == win_len_;
    }

    //! Returns the moving quantile.
    //! @note
    //!  Has O(1) complexity.
    T mov_quantile() const {
        roc_panic_if(!valid_);

        return heap_[heap_root_];
    }

    //! Inserts or swaps elements in the partition heap.
    //! @remarks
    //!  Case 1: The window is filled. The element in heap is changed whose
    //!    element_index%window_length is equal to arrived element. heapify_() is
    //!    called post that.
    //!  Case 2: The window in not filled. In this case we insert element in max_heap,
    //!    min_heap or root based on the current percentile index.
    //! @note
    //!  Has O(log(win_len)) complexity.
    void add(const T& x) {
        roc_panic_if(!valid_);

        const bool win_filled = (heap_size_ == win_len_);
        if (heap_size_ < win_len_) {
            heap_size_++;
        }

        if (win_filled) {
            const size_t heap_index = elem_index_2_heap_index_[elem_index_];
            heap_[heap_index] = x;
            heapify_(heap_index);
        } else {
            const size_t k = size_t(quantile_ * double(heap_size_ - 1));
            if (elem_index_ == 0) {
                const size_t heap_index = heap_root_;
                elem_index_2_heap_index_[elem_index_] = heap_index;
                heap_[heap_index] = x;
                heap_index_2_elem_index_[heap_index] = elem_index_;
            } else {
                size_t heap_index;
                if (old_heap_root_index_ == k) {
                    min_heap_index_ += 1;
                    heap_index = min_heap_index_;
                } else {
                    max_heap_index_ -= 1;
                    heap_index = max_heap_index_;
                }
                elem_index_2_heap_index_[elem_index_] = heap_index;
                heap_[heap_index] = x;
                heap_index_2_elem_index_[heap_index] = elem_index_;
                heapify_(heap_index);
                old_heap_root_index_ = k;
            }
        }

        elem_index_ = (elem_index_ + 1) % win_len_;
    }

private:
    // Maintains property of the partition heap when an element in inserted or swapped.
    // The element could be inserted or changed in min_heap, max_heap or the root.
    void heapify_(const size_t heap_index) {
        if (heap_index < heap_root_) {
            const size_t parent = heap_root_ - ((heap_root_ - heap_index - 1) / 2);
            if (heap_[parent] < heap_[heap_index]) {
                max_heapify_up_(heap_index);
                min_heapify_down_(heap_root_);
            } else {
                max_heapify_down_(heap_index);
            }
        } else if (heap_root_ == heap_index) {
            max_heapify_down_(heap_index);
            min_heapify_down_(heap_root_);
        } else {
            const size_t parent = (heap_index - heap_root_ - 1) / 2 + heap_root_;
            if (heap_[parent] > heap_[heap_index]) {
                min_heapify_up_(heap_index);
                max_heapify_down_(heap_root_);
            } else {
                min_heapify_down_(heap_index);
            }
        }
    }

    // Recursively swaps parent and element in min heap partition until the parent is
    // smaller or element reaches root index.
    void min_heapify_up_(const size_t heap_index) {
        if (heap_index == heap_root_) {
            return;
        }
        const size_t parent = (heap_index - heap_root_ - 1) / 2 + heap_root_;
        if (heap_[parent] > heap_[heap_index]) {
            swap_(heap_index, parent);
            min_heapify_up_(parent);
        }
    }

    // Recursively swaps parent and element in max heap partition until the parent is
    // larger or element reaches root index.
    // The root index in max heap partition is larger than all its child index so parent
    // index formulae has been adjusted accordingly
    void max_heapify_up_(const size_t heap_index) {
        // sift up
        if (heap_index == heap_root_) {
            return;
        }
        const size_t parent = heap_root_ - ((heap_root_ - heap_index - 1) / 2);
        if (heap_[parent] < heap_[heap_index]) {
            swap_(heap_index, parent);
            max_heapify_up_(parent);
        }
    }

    // Recursively swaps children and element in min heap partition until the children
    // are smaller or there are no children
    void min_heapify_down_(const size_t heap_index) {
        size_t largest = heap_index;

        const size_t left = 2 * (heap_index - heap_root_) + 1 + heap_root_;
        if (left <= min_heap_index_ && heap_[left] < heap_[largest]) {
            largest = left;
        }
        const size_t right = 2 * (heap_index - heap_root_) + 2 + heap_root_;
        if (right <= min_heap_index_ && heap_[right] < heap_[largest]) {
            largest = right;
        }

        if (largest != heap_index) {
            swap_(heap_index, largest);
            min_heapify_down_(largest);
        }
    }

    // Recursively swaps children and element in max heap partition until the children
    // are larger or there are no children.
    // Similar adjustment to child index calculation like in max_heapify_up
    void max_heapify_down_(const size_t heap_index) {
        size_t largest = heap_index;

        const size_t left = 2 * (heap_root_ - heap_index) + 1;
        if (left <= (heap_root_ - max_heap_index_)
            && heap_[heap_root_ - left] > heap_[largest]) {
            largest = heap_root_ - left;
        }
        const size_t right = 2 * (heap_root_ - heap_index) + 2;
        if (right <= (heap_root_ - max_heap_index_)
            && heap_[heap_root_ - right] > heap_[largest]) {
            largest = heap_root_ - right;
        }
        if (largest != heap_index) {
            swap_(heap_index, largest);
            max_heapify_down_(largest);
        }
    }

    // Swaps 2 heap elements along with their mapping in element to heap index and heap
    // to element index
    void swap_(const size_t index_1, const size_t index_2) {
        const size_t elem_index_1 = heap_index_2_elem_index_[index_1];
        const size_t elem_index_2 = heap_index_2_elem_index_[index_2];

        const T temp = heap_[index_1];
        heap_[index_1] = heap_[index_2];
        heap_[index_2] = temp;

        heap_index_2_elem_index_[index_1] = elem_index_2;
        heap_index_2_elem_index_[index_2] = elem_index_1;

        elem_index_2_heap_index_[elem_index_1] = index_2;
        elem_index_2_heap_index_[elem_index_2] = index_1;
    }

    // Length of the sliding window
    const size_t win_len_;
    // Quantile of the window elements
    const double quantile_;

    // Used to check the window filling logic
    size_t old_heap_root_index_;
    // Index which separates max and min heap and also act as their root
    size_t heap_root_;

    // Maintains current heap size
    size_t heap_size_;
    // Maintains the index to which max_heap extends
    size_t max_heap_index_;
    // Maintains the index to which min_heap extends
    size_t min_heap_index_;

    // Maintains current element index
    size_t elem_index_;

    // Maintains the partition heap
    core::Array<T> heap_;
    // Maintains the element index to heap index mapping
    core::Array<size_t> elem_index_2_heap_index_;
    // Maintains the heap index to element index mapping
    core::Array<size_t> heap_index_2_elem_index_;

    bool valid_;
};

} // namespace stat
} // namespace roc

#endif // ROC_STAT_MOV_QUANTILE_H_
