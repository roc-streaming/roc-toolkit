/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/mov_stats.h
//! @brief Profiler.

#ifndef ROC_CORE_MOV_STATS_H_
#define ROC_CORE_MOV_STATS_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/panic.h"
#include "roc_core/queue.h"

namespace roc {
namespace core {

//! Rolling window moving average and variance.
//! @remarks
//! Efficiently implements moving average and variance based on approach
//! described in https://www.dsprelated.com/showthread/comp.dsp/97276-1.php
//!
//! @tparam T defines a sample type.
//!
//! @note T should be trivially copyable.
template <typename T> class MovStats {
public:
    //! Initialize.
    MovStats(IArena& arena, const size_t win_len)
        : buffer_(arena)
        , buffer2_(arena)
        , win_len_(win_len)
        , buffer_i_(0)
        , movsum_(T(0))
        , movsum2_(T(0))
        , mov_var_(T(0))
        , mov_max_cntr_(0)
        , full_(false)
        , first_(true)
        , queue_max_(arena, win_len + 1)
        , curr_max_(T(0))
        , queue_min_(arena, win_len + 1)
        , curr_min_(T(0)) {
        if (win_len == 0) {
            roc_panic("mov stats: window length must be greater than 0");
        }

        if (!buffer_.resize(win_len)) {
            return;
        }
        if (!buffer2_.resize(win_len)) {
            return;
        }
        memset(buffer_.data(), 0, sizeof(T) * buffer_.size());
        memset(buffer2_.data(), 0, sizeof(T) * buffer2_.size());
    }

    //! Shift rolling window by one sample x.
    void add(const T& x) {
        const T x2 = x * x;
        const T x_old = buffer_[buffer_i_];
        buffer_[buffer_i_] = x;
        const T x2_old = buffer2_[buffer_i_];
        buffer2_[buffer_i_] = x2;

        movsum_ += x - x_old;
        movsum2_ += x2 - x2_old;

        buffer_i_++;
        if (buffer_i_ == win_len_) {
            buffer_i_ = 0;
            full_ = true;
        }

        slide_max_(x, x_old);
        slide_min_(x, x_old);
    }

    //! Get moving average value.
    T mov_avg() const {
        T n = 0;
        if (full_) {
            n = T(win_len_);
        } else if (buffer_i_ == 0) {
            return T(0);
        } else {
            n = T(buffer_i_);
        }
        return movsum_ / n;
    }

    //! Get variance.
    T mov_var() const {
        T n = 0;
        if (full_) {
            n = T(win_len_);
        } else if (buffer_i_ == 0) {
            return T(0);
        } else {
            n = T(buffer_i_);
        }
        return (T)sqrt((n * movsum2_ - movsum_ * movsum_) / (n * n));
    }

    //! Max value in sliding window.
    T mov_max() const {
        return curr_max_;
    }

    //! Min value in sliding window.
    T mov_min() const {
        return curr_min_;
    }

    //! Extend rolling window length.
    //! @remarks
    //! Potentially could cause a gap in the estimated values as
    //! decreases effective window size by dropping samples to the right from
    //! the cursor in the ring buffers:
    //!          buffer_i_        win_len_ old       win_len_ new
    //!             ↓                    ↓                  ↓
    //!  [■■■■■■■■■■□□□□□□□□□□□□□□□□□□□□□--------------------]
    //!             ↑         ↑          ↑
    //!                Dropped samples.
    ROC_ATTR_NODISCARD bool extend_win(const size_t new_win) {
        if (new_win <= win_len_) {
            roc_panic("mov stats: the window length can only grow");
        }
        if (!buffer_.resize(new_win)) {
            return false;
        }
        if (!buffer2_.resize(new_win)) {
            return false;
        }

        movsum_ = 0;
        movsum2_ = 0;
        for (size_t i = 0; i < buffer_i_; i++) {
            movsum_ += buffer_[i];
            movsum2_ += buffer2_[i];
        }
        full_ = false;
        return true;
    }

    //! Check that initial allocation succeeded.
    bool is_valid() const {
        return buffer_.data() && buffer2_.data();
    }

private:
    //! Keeping a sliding max by using a sorted deque.
    //! @remarks
    //! The wedge is always sorted in descending order.
    //! The current max is always at the front of the wedge.
    //! https://www.geeksforgeeks.org/sliding-window-maximum-maximum-of-all-subarrays-of-size-k/
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

    //! Keeping a sliding min by using a sorted deque.
    //! @remarks
    //! The wedge is always sorted in ascending order.
    //! The current min is always at the front of the wedge.
    //! https://www.geeksforgeeks.org/sliding-window-maximum-maximum-of-all-subarrays-of-size-k/
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

    Array<T> buffer_;
    Array<T> buffer2_;

    const size_t win_len_;
    size_t buffer_i_;
    T movsum_;
    T movsum2_;
    T mov_var_;
    T mov_max_;
    size_t mov_max_cntr_;

    bool full_;
    bool first_;

    Queue<T> queue_max_;
    T curr_max_;
    Queue<T> queue_min_;
    T curr_min_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MOV_STATS_H_
