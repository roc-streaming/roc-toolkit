/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_stat/mov_aggregate.h
//! @brief Rolling window average, variance, minimum, maximum.

#ifndef ROC_STAT_MOV_AGGREGATE_H_
#define ROC_STAT_MOV_AGGREGATE_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/panic.h"
#include "roc_core/ring_queue.h"

namespace roc {
namespace stat {

//! Rolling window average, variance, minimum, maximum.
//!
//! Efficiently implements moving average and variance based on Welford's method:
//!  - https://www.johndcook.com/blog/standard_deviation (incremental)
//!  - https://stackoverflow.com/a/6664212/3169754 (rolling window)
//!
//! And moving minimum/maximum based on "sorted deque" algorithm:
//!  https://www.geeksforgeeks.org/sliding-window-maximum-maximum-of-all-subarrays-of-size-k/
//!
//! @tparam T defines a sample type.
template <typename T> class MovAggregate {
public:
    //! Initialize.
    MovAggregate(core::IArena& arena, const size_t win_len)
        : win_len_(win_len)
        , buffer_(arena)
        , buffer_i_(0)
        , movmean_(0)
        , movvar_(0)
        , full_(false)
        , queue_max_(arena, win_len)
        , curr_max_(T(0))
        , queue_min_(arena, win_len)
        , curr_min_(T(0))
        , valid_(false) {
        if (win_len == 0) {
            roc_panic("mov stats: window length must be greater than 0");
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

    //! Get moving average.
    //! @note
    //!  Has O(1) complexity.
    T mov_avg() const {
        roc_panic_if(!valid_);

        T ret;
        double_2_t_(movmean_, ret);
        return ret;
    }

    //! Get moving variance.
    //! @note
    //!  Has O(1) complexity.
    T mov_var() const {
        roc_panic_if(!valid_);

        T ret;
        double_2_t_(movvar_ > 0 ? movvar_ : 0, ret);
        return ret;
    }

    //! Get moving standard deviation.
    //! @note
    //!  Has O(1) complexity.
    T mov_std() const {
        roc_panic_if(!valid_);

        T ret;
        double_2_t_(sqrt(movvar_ > 0 ? movvar_ : 0), ret);
        return ret;
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

        update_sums_(x, x_old);

        buffer_i_++;
        if (buffer_i_ == win_len_) {
            buffer_i_ = 0;
            full_ = true;
        }

        slide_max_(x, x_old);
        slide_min_(x, x_old);
    }

private:
    // Update moving average and moving variance.
    void update_sums_(const T& x, const T x_old) {
        if (full_) {
            // Since window is full, use rolling window adaption of Welford's method.
            // Operations are reordered to avoid overflows.
            const double movmean_old = movmean_;
            movmean_ += double(x - x_old) / win_len_;
            movvar_ += ((x - movmean_) + (x_old - movmean_old)) / win_len_ * (x - x_old);
        } else {
            // Until window is full, use original Welford's method.
            // Operations are reordered to avoid overflows.
            const double movmean_old = movmean_;
            const double n = buffer_i_;
            movmean_ += (x - movmean_) / (n + 1);
            if (n > 0) {
                movvar_ =
                    (movvar_ + (x - movmean_old) / n * (x - movmean_)) * (n / (n + 1));
            }
        }
    }

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

    double movmean_;
    double movvar_;

    bool full_;

    core::RingQueue<T> queue_max_;
    T curr_max_;
    core::RingQueue<T> queue_min_;
    T curr_min_;

    bool valid_;
};

} // namespace stat
} // namespace roc

#endif // ROC_STAT_MOV_AGGREGATE_H_
