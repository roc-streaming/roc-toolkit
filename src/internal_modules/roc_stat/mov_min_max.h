/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_stat/mov_min_max.h
//! @brief Rolling window minimum and maximum.

#ifndef ROC_STAT_MOV_MIN_MAX_H_
#define ROC_STAT_MOV_MIN_MAX_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/panic.h"
#include "roc_core/ring_queue.h"

namespace roc {
namespace stat {

//! Rolling window minimum and maximum.
//!
//! Implements moving minimum/maximum based on "sorted deque" algorithm from here:
//!  https://www.geeksforgeeks.org/sliding-window-maximum-maximum-of-all-subarrays-of-size-k/
//!
//! @tparam T defines a sample type.
template <typename T> class MovMinMax {
public:
    //! Initialize.
    MovMinMax(core::IArena& arena, const size_t win_len)
        : win_len_(win_len)
        , buffer_(arena)
        , buffer_i_(0)
        , full_(false)
        , queue_max_(arena, win_len)
        , curr_max_(T(0))
        , queue_min_(arena, win_len)
        , curr_min_(T(0))
        , valid_(false) {
        if (win_len == 0) {
            roc_panic("mov min max: window length must be greater than 0");
        }

        if (!queue_max_.is_valid() || !queue_min_.is_valid()) {
            return;
        }
        if (!buffer_.resize(win_len)) {
            return;
        }

        valid_ = true;
    }

    //! Check that initial allocation succeeded.
    bool is_valid() const {
        return valid_;
    }

    //! Check if the window is fully filled.
    size_t is_full() const {
        return full_;
    }

    //! Min value in sliding window.
    //! @note
    //!  Has O(1) complexity.
    T mov_min() const {
        roc_panic_if(!valid_);

        return curr_min_;
    }

    //! Max value in sliding window.
    //! @note
    //!  Has O(1) complexity.
    T mov_max() const {
        roc_panic_if(!valid_);

        return curr_max_;
    }

    //! Shift rolling window by one sample x.
    //! @note
    //!  Has O(win_len) complexity.
    void add(const T& x) {
        roc_panic_if(!valid_);

        const T x_old = buffer_[buffer_i_];
        buffer_[buffer_i_] = x;

        buffer_i_++;
        if (buffer_i_ == win_len_) {
            buffer_i_ = 0;
            full_ = true;
        }

        slide_max_(x, x_old);
        slide_min_(x, x_old);
    }

private:
    // Keeping a sliding max by using a sorted deque.
    // The wedge is always sorted in descending order.
    // The current max is always at the front of the wedge.
    void slide_max_(const T& x, const T x_old) {
        if (queue_max_.is_empty()) {
            queue_max_.push_back(x);
            curr_max_ = x;
        } else {
            if (queue_max_.front() == x_old) {
                queue_max_.pop_front();
                curr_max_ = queue_max_.is_empty() ? x : queue_max_.front();
            }
            while (!queue_max_.is_empty() && queue_max_.back() < x) {
                queue_max_.pop_back();
            }
            if (queue_max_.is_empty()) {
                curr_max_ = x;
            }
            queue_max_.push_back(x);
        }
    }

    // Keeping a sliding min by using a sorted deque.
    // The wedge is always sorted in ascending order.
    // The current min is always at the front of the wedge.
    void slide_min_(const T& x, const T x_old) {
        if (queue_min_.is_empty()) {
            queue_min_.push_back(x);
            curr_min_ = x;
        } else {
            if (queue_min_.front() == x_old) {
                queue_min_.pop_front();
                curr_min_ = queue_min_.is_empty() ? x : queue_min_.front();
            }
            while (!queue_min_.is_empty() && queue_min_.back() > x) {
                queue_min_.pop_back();
            }
            if (queue_min_.is_empty()) {
                curr_min_ = x;
            }
            queue_min_.push_back(x);
        }
    }

    // Convert double to sample type.
    // If sample type is integer, use round(), otherwise regular cast.
    template <class TT> void double_2_t_(double in, TT& out) const {
        out = (TT)round(in);
    }
    void double_2_t_(double in, float& out) const {
        out = (float)in;
    }
    void double_2_t_(double in, double& out) const {
        out = in;
    }

    const size_t win_len_;

    core::Array<T> buffer_;
    size_t buffer_i_;

    bool full_;

    core::RingQueue<T> queue_max_;
    T curr_max_;
    core::RingQueue<T> queue_min_;
    T curr_min_;

    bool valid_;
};

} // namespace stat
} // namespace roc

#endif // ROC_STAT_MOV_MIN_MAX_H_
