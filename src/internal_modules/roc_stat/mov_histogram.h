/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_stat/mov_histogram.h
//! @brief Rolling window histogram.

#ifndef ROC_STAT_MOV_HISTOGRAM_H_
#define ROC_STAT_MOV_HISTOGRAM_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/panic.h"
#include "roc_core/ring_queue.h"

namespace roc {
namespace stat {

//! Rolling window histogram.
//!
//! The MovHistogram class maintains a histogram of values within a specified window
//! length. It divides the range of values into a specified number of bins and updates the
//! histogram as new values are added and old values are removed from the window.
//!
//! Similar to MovQuantile, this class also is capable of computing moving quantiles.
//! MovHistogram is generally faster than MovQuantile, but has several restrictions:
//!  - value range should be limited and relatively small compared to the bin size;
//!    you need either small range or large bins
//!  - calculated quantile is only an approximation, and error depends on bin size;
//!    you need small bins for better precision
//!  - calculation of quantile has O(N) complexity based on the number of bins;
//!    you need lesser bins to keep it fast
//!
//! @tparam T The type of values to be histogrammed.
template <typename T> class MovHistogram {
public:
    //! Constructs a moving histogram.
    //! @param arena Memory arena for dynamic allocations.
    //! @param value_range_min The minimum value of the range to be histogrammed.
    //! @param value_range_max The maximum value of the range to be histogrammed.
    //! (values outside of the range are clamped to the range boundaries).
    //! @param num_bins The number of bins in the histogram. Each bin represents a
    //! subrange of the value range.
    //! @param window_length The length of the moving window. Only values within this
    //! window are considered in the histogram.
    MovHistogram(core::IArena& arena,
                 T value_range_min,
                 T value_range_max,
                 size_t num_bins,
                 size_t window_length)
        : value_range_min_(value_range_min)
        , value_range_max_(value_range_max)
        , num_bins_(num_bins)
        , window_length_(window_length)
        , ring_buffer_(arena, window_length)
        , bins_(arena)
        , valid_(false) {
        roc_panic_if_msg(window_length == 0, "mov histogram: window_length must be > 0");
        roc_panic_if_msg(num_bins == 0, "mov histogram: num_bins must be > 0");
        roc_panic_if_msg(
            value_range_min >= value_range_max,
            "mov histogram: value_range_min must be less than value_range_max");

        bin_width_ = (value_range_max - value_range_min) / T(num_bins);

        if (!ring_buffer_.is_valid() || !bins_.resize(num_bins)) {
            return;
        }

        valid_ = true;
    }

    //! Check if the histogram is valid.
    bool is_valid() const {
        return valid_;
    }

    //! Check if the window is fully filled.
    size_t is_full() const {
        return ring_buffer_.is_full();
    }

    //! Get the number of values in the given bin.
    //! @note
    //!  Has O(1) complexity.
    size_t mov_counter(size_t bin_index) const {
        roc_panic_if(!valid_);

        return bins_[bin_index];
    }

    //! Get approximated moving quantile.
    //! @note
    //!  Has O(num_bins) complexity.
    T mov_quantile(const double quantile) const {
        roc_panic_if(!valid_);

        T cap = T(0);
        size_t count = 0;

        for (size_t bin_index = 0; bin_index < num_bins_; bin_index++) {
            cap = value_range_min_ + T(bin_width_) * T(bin_index + 1);
            count += bins_[bin_index];

            const double ratio = (double)count / ring_buffer_.size();
            if (ratio >= quantile) {
                break;
            }
        }

        return cap;
    }

    //! Add a value to the histogram.
    //! @note
    //!  Has O(1) complexity.
    void add(const T& value) {
        roc_panic_if(!valid_);

        T clamped_value = value;

        if (clamped_value < value_range_min_) {
            clamped_value = value_range_min_;
        } else if (clamped_value > value_range_max_) {
            clamped_value = value_range_max_;
        }

        if (ring_buffer_.size() == window_length_) {
            const T oldest_value = ring_buffer_.front();
            ring_buffer_.pop_front();
            const size_t oldest_bin_index = get_bin_index_(oldest_value);
            bins_[oldest_bin_index]--;
        }

        ring_buffer_.push_back(clamped_value);
        const size_t new_bin_index = get_bin_index_(clamped_value);
        if (new_bin_index < num_bins_) {
            bins_[new_bin_index]++;
        }
    }

private:
    //! Get the bin index for the given value.
    size_t get_bin_index_(const T& value) const {
        if (value == value_range_max_) {
            return num_bins_ - 1;
        }

        return size_t((value - value_range_min_) / bin_width_);
    }

    const T value_range_min_;
    const T value_range_max_;
    const size_t num_bins_;
    const size_t window_length_;
    T bin_width_;

    core::RingQueue<T> ring_buffer_;
    core::Array<size_t> bins_;

    bool valid_;
};

} // namespace stat
} // namespace roc

#endif // ROC_STAT_MOV_HISTOGRAM_H_
