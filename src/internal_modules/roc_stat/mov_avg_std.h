/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_stat/mov_avg_std.h
//! @brief Rolling window average and standard deviation.

#ifndef ROC_STAT_MOV_AVG_STD_H_
#define ROC_STAT_MOV_AVG_STD_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/panic.h"

namespace roc {
namespace stat {

//! Rolling window average and standard deviation.
//!
//! Efficiently implements moving average and variance based on Welford's method:
//!  - https://www.johndcook.com/blog/standard_deviation (incremental)
//!  - https://stackoverflow.com/a/6664212/3169754 (rolling window)
//!
//! @tparam T defines a sample type.
template <typename T> class MovAvgStd {
public:
    //! Initialize.
    MovAvgStd(core::IArena& arena, const size_t win_len)
        : win_len_(win_len)
        , buffer_(arena)
        , buffer_i_(0)
        , movmean_(0)
        , movvar_(0)
        , full_(false)
        , valid_(false) {
        if (win_len == 0) {
            roc_panic("mov avg std: window length must be greater than 0");
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

    //! Shift rolling window by one sample x.
    //! @note
    //!  Has O(1) complexity.
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

    bool valid_;
};

} // namespace stat
} // namespace roc

#endif // ROC_STAT_MOV_AVG_STD_H_
