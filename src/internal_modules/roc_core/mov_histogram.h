/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/mov_histogram.h
//! @brief Rolling window moving histogram.

#ifndef ROC_CORE_MOV_HISTOGRAM_H_
#define ROC_CORE_MOV_HISTOGRAM_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/panic.h"
#include "roc_core/ring_queue.h"

namespace roc {
namespace core {

//! @brief A class that implements a rolling window moving histogram.
//! The MovHistogram class maintains a histogram of values within a specified window
//! length. It divides the range of values into a specified number of bins and updates the
//! histogram as new values are added and old values are removed from the window.
//! @tparam T The type of values to be histogrammed.

template <typename T> class MovHistogram {
public:
    //! @brief Constructs a moving histogram.
    //! @param arena Memory arena for dynamic allocations.
    //! @param value_range_min The minimum value of the range to be histogrammed.
    //! @param value_range_max The maximum value of the range to be histogrammed.
    //! @param num_bins The number of bins in the histogram. Each bin represents a
    //! subrange of the value range.
    //! @param window_length The length of the moving window. Only values within this
    //! window are considered in the histogram.

    MovHistogram(IArena& arena,
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
        if (num_bins == 0 || window_length == 0 || value_range_min >= value_range_max) {
            roc_panic("mov histogram: number of bins and window length must be greater "
                      "than 0 and value_range_min must be less than value_range_max");
        }

        bin_width_ = (value_range_max - value_range_min) / static_cast<T>(num_bins);

        if (!ring_buffer_.is_valid() || !bins_.resize(num_bins)) {
            return;
        }

        valid_ = true;
    }

    //! Check if the histogram is valid.
    bool is_valid() const {
        return valid_;
    }

    //! Add a value to the histogram.
    void add_value(const T& value) {
        T clamped_value = value;

        if (clamped_value < value_range_min_) {
            clamped_value = value_range_min_;
        } else if (clamped_value > value_range_max_) {
            clamped_value = value_range_max_;
        }

        if (ring_buffer_.size() == window_length_) {
            T oldest_value = ring_buffer_.front();
            ring_buffer_.pop_front();
            size_t oldest_bin_index = get_bin_index_(oldest_value);
            bins_[oldest_bin_index]--;
        }

        ring_buffer_.push_back(clamped_value);
        size_t new_bin_index = get_bin_index_(clamped_value);
        if (new_bin_index < num_bins_) {
            bins_[new_bin_index]++;
        }
    }

    //! Get the number of values in the given bin.
    size_t get_bin_counter(size_t bin_index) const {
        return bins_[bin_index];
    }

private:
    //! Get the bin index for the given value.
    size_t get_bin_index_(const T& value) const {
        if (value == value_range_max_) {
            return num_bins_ - 1;
        }

        return static_cast<size_t>((value - value_range_min_) / bin_width_);
    }

    T value_range_min_;
    T value_range_max_;
    size_t num_bins_;
    size_t window_length_;
    T bin_width_;
    RingQueue<T> ring_buffer_;
    Array<size_t> bins_;
    bool valid_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_MOV_HISTOGRAM_H_
