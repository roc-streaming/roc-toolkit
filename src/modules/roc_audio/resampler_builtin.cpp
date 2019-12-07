/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_builtin.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

//! Fixed point type Q8.24 for realizing computations of curr_frame_ in fixed point
//! arithmetic. Sometimes this computations requires ceil(...) and floor(...) and
//! it is very CPU-time hungry in floating point variant on x86.
typedef uint32_t fixedpoint_t;

// Signed version of fixedpoint_t.
typedef int32_t signed_fixedpoint_t;

const uint32_t INTEGER_PART_MASK = 0xFFF00000;
const uint32_t FRACT_PART_MASK = 0x000FFFFF;
const uint32_t FRACT_BIT_COUNT = 20;

// One in terms of Q8.24.
const fixedpoint_t qt_one = 1 << FRACT_BIT_COUNT;

// Convert float to fixed-point.
inline fixedpoint_t float_to_fixedpoint(const float t) {
    return (fixedpoint_t)(t * (float)qt_one);
}

inline size_t fixedpoint_to_size(const fixedpoint_t t) {
    return t >> FRACT_BIT_COUNT;
}

// Rounds x (Q8.24) upward.
inline fixedpoint_t qceil(const fixedpoint_t x) {
    if ((x & FRACT_PART_MASK) == 0) {
        return x & INTEGER_PART_MASK;
    } else {
        return (x & INTEGER_PART_MASK) + qt_one;
    }
}

// Rounds x (Q8.24) downward.
inline fixedpoint_t qfloor(const fixedpoint_t x) {
    // Just remove fractional part.
    return x & INTEGER_PART_MASK;
}

// Returns fractional part of x in f32.
inline float fractional(const fixedpoint_t x) {
    return (float)(x & FRACT_PART_MASK) * ((float)1. / (float)qt_one);
}

// Returns log2(n) assuming that n is a power of two.
inline size_t calc_bits(size_t n) {
    size_t c = 0;
    while ((n & 1) == 0 && c != sizeof(n) * 8) {
        n >>= 1;
        c++;
    }
    return c;
}

} // namespace

BuiltinResampler::BuiltinResampler(core::IAllocator& allocator,
                                   const ResamplerConfig& config,
                                   core::nanoseconds_t frame_length,
                                   size_t sample_rate,
                                   packet::channel_mask_t channels)
    : channel_mask_(channels)
    , channels_num_(packet::num_channels(channel_mask_))
    , prev_frame_(NULL)
    , curr_frame_(NULL)
    , next_frame_(NULL)
    , out_frame_pos_(0)
    , scaling_(1.0)
    , frame_size_(packet::ns_to_size(frame_length, sample_rate, channels))
    , frame_size_ch_(channels_num_ ? frame_size_ / channels_num_ : 0)
    , window_size_(config.window_size)
    , qt_half_sinc_window_size_(float_to_fixedpoint(window_size_))
    , window_interp_(config.window_interp)
    , window_interp_bits_(calc_bits(config.window_interp))
    , sinc_table_(allocator)
    , sinc_table_ptr_(NULL)
    , qt_half_window_size_(float_to_fixedpoint((float)window_size_ / scaling_))
    , qt_epsilon_(float_to_fixedpoint(5e-8f))
    , qt_frame_size_(fixedpoint_t(frame_size_ch_ << FRACT_BIT_COUNT))
    , qt_sample_(float_to_fixedpoint(0))
    , qt_dt_(0)
    , cutoff_freq_(0.9f)
    , valid_(false) {
    if (!check_config_()) {
        return;
    }
    if (!fill_sinc_()) {
        return;
    }

    roc_log(LogDebug,
            "resampler: initializing: "
            "window_interp=%lu window_size=%lu frame_size=%lu channels_num=%lu",
            (unsigned long)window_interp_, (unsigned long)window_size_,
            (unsigned long)frame_size_, (unsigned long)channels_num_);

    valid_ = true;
}

BuiltinResampler::~BuiltinResampler() {
}

bool BuiltinResampler::valid() const {
    return valid_;
}

bool BuiltinResampler::set_scaling(size_t input_sample_rate,
                                   size_t output_sample_rate,
                                   float multiplier) {
    const float new_scaling = float(input_sample_rate) / output_sample_rate * multiplier;

    // Window's size changes according to scaling. If new window size
    // doesn't fit to the frames size -- deny changes.
    if (window_size_ * new_scaling >= frame_size_ch_) {
        roc_log(LogError,
                "resampler: scaling does not fit frame size:"
                " window_size=%lu frame_size=%lu scaling=%.5f",
                (unsigned long)window_size_, (unsigned long)frame_size_,
                (double)new_scaling);
        return false;
    }

    // In case of upscaling one should properly shift the edge frequency
    // of the digital filter. In both cases it's sensible to decrease the
    // edge frequency to leave some.
    if (new_scaling > 1.0f) {
        const fixedpoint_t new_qt_half_window_len =
            float_to_fixedpoint((float)window_size_ / cutoff_freq_ * new_scaling);

        // Check that resample_() will not go out of bounds.
        // Otherwise -- deny changes.
        const bool out_of_bounds =
            fixedpoint_to_size(qceil(qt_frame_size_ - new_qt_half_window_len))
                > frame_size_ch_
            || fixedpoint_to_size(qfloor(new_qt_half_window_len)) + 1 > frame_size_ch_;

        if (out_of_bounds) {
            roc_log(LogError,
                    "resampler: scaling does not fit window size:"
                    " window_size=%lu frame_size=%lu scaling=%.5f",
                    (unsigned long)window_size_, (unsigned long)frame_size_,
                    (double)new_scaling);
            return false;
        }

        qt_sinc_step_ = float_to_fixedpoint(cutoff_freq_ / new_scaling);
        qt_half_window_size_ = new_qt_half_window_len;
    } else {
        qt_sinc_step_ = float_to_fixedpoint(cutoff_freq_);
        qt_half_window_size_ = float_to_fixedpoint((float)window_size_ / cutoff_freq_);
    }

    scaling_ = new_scaling;

    return true;
}

bool BuiltinResampler::resample_buff(Frame& out) {
    roc_panic_if(!prev_frame_);
    roc_panic_if(!curr_frame_);
    roc_panic_if(!next_frame_);

    for (; out_frame_pos_ < out.size(); out_frame_pos_ += channels_num_) {
        if (qt_sample_ >= qt_frame_size_) {
            return false;
        }

        if ((qt_sample_ & FRACT_PART_MASK) < qt_epsilon_) {
            qt_sample_ &= INTEGER_PART_MASK;
        } else if ((qt_one - (qt_sample_ & FRACT_PART_MASK)) < qt_epsilon_) {
            qt_sample_ &= INTEGER_PART_MASK;
            qt_sample_ += qt_one;
        }

        sample_t* out_data = out.data();
        for (size_t channel = 0; channel < channels_num_; ++channel) {
            out_data[out_frame_pos_ + channel] = resample_(channel);
        }
        qt_sample_ += qt_dt_;
    }
    out_frame_pos_ = 0;
    return true;
}

bool BuiltinResampler::check_config_() const {
    if (channels_num_ < 1) {
        roc_log(LogError, "resampler: invalid num_channels: num_channels=%lu",
                (unsigned long)channels_num_);
        return false;
    }

    if (frame_size_ != frame_size_ch_ * channels_num_) {
        roc_log(LogError,
                "resampler: frame_size is not multiple of num_channels:"
                " frame_size=%lu num_channels=%lu",
                (unsigned long)frame_size_, (unsigned long)channels_num_);
        return false;
    }

    const size_t max_frame_size =
        (((fixedpoint_t)(signed_fixedpoint_t)-1 >> FRACT_BIT_COUNT) + 1) * channels_num_;
    if (frame_size_ > max_frame_size) {
        roc_log(LogError,
                "resampler: frame_size is too much: "
                "max_frame_size=%lu frame_size=%lu num_channels=%lu",
                (unsigned long)max_frame_size, (unsigned long)frame_size_,
                (unsigned long)channels_num_);
        return false;
    }

    if ((size_t)1 << window_interp_bits_ != window_interp_) {
        roc_log(LogError,
                "resampler: window_interp is not power of two: window_interp=%lu",
                (unsigned long)window_interp_);
        return false;
    }

    return true;
}

void BuiltinResampler::renew_buffers(core::Slice<sample_t>& prev,
                                     core::Slice<sample_t>& cur,
                                     core::Slice<sample_t>& next) {
    roc_panic_if(window_size_ * scaling_ >= frame_size_ch_);

    roc_panic_if(prev.size() != frame_size_);
    roc_panic_if(cur.size() != frame_size_);
    roc_panic_if(next.size() != frame_size_);

    if (qt_sample_ >= qt_frame_size_) {
        qt_sample_ -= qt_frame_size_;
    }

    // scaling_ may change every frame so it have to be smooth
    qt_dt_ = float_to_fixedpoint(scaling_);

    prev_frame_ = prev.data();
    curr_frame_ = cur.data();
    next_frame_ = next.data();
}

bool BuiltinResampler::fill_sinc_() {
    if (!sinc_table_.resize(window_size_ * window_interp_ + 2)) {
        roc_log(LogError, "resampler: can't allocate sinc table");
        return false;
    }

    const double sinc_step = 1.0 / (double)window_interp_;
    double sinc_t = sinc_step;

    sinc_table_[0] = 1.0f;
    for (size_t i = 1; i < sinc_table_.size(); ++i) {
        const double window = 0.54
            - 0.46
                * std::cos(2 * M_PI
                           * ((double)(i - 1) / 2.0 / (double)sinc_table_.size() + 0.5));
        sinc_table_[i] = (float)(std::sin(M_PI * sinc_t) / M_PI / sinc_t * window);
        sinc_t += sinc_step;
    }
    sinc_table_[sinc_table_.size() - 2] = 0;
    sinc_table_[sinc_table_.size() - 1] = 0;

    sinc_table_ptr_ = &sinc_table_[0];

    return true;
}

// Computes sinc value in x position using linear interpolation between
// table values from sinc_table.h
//
// During going through input signal window only integer part of argument changes,
// that's why there are two arguments in this function: integer part and fractional
// part of time coordinate.
sample_t BuiltinResampler::sinc_(const fixedpoint_t x, const float fract_x) {
    const size_t index = (x >> (FRACT_BIT_COUNT - window_interp_bits_));

    const sample_t hl = sinc_table_ptr_[index];     // table index smaller than x
    const sample_t hh = sinc_table_ptr_[index + 1]; // table index next to x

    const sample_t result = hl + fract_x * (hh - hl);

    return scaling_ > 1.0f ? result / scaling_ : result;
}

sample_t BuiltinResampler::resample_(const size_t channel_offset) {
    // Index of first input sample in window.
    size_t ind_begin_prev;

    // Window lasts till that index.
    const size_t ind_end_prev = channelize_index(frame_size_ch_, channel_offset);

    size_t ind_begin_cur;
    size_t ind_end_cur;

    const size_t ind_begin_next = channelize_index(0, channel_offset);
    size_t ind_end_next;

    ind_begin_prev = (qt_sample_ >= qt_half_window_size_)
        ? frame_size_ch_
        : fixedpoint_to_size(qceil(qt_sample_ + (qt_frame_size_ - qt_half_window_size_)));
    // ind_begin_prev is comparable with channel_len_ till we'll convert it to channalyzed
    // presentation.
    roc_panic_if(ind_begin_prev > frame_size_ch_);
    ind_begin_prev = channelize_index(ind_begin_prev, channel_offset);

    ind_begin_cur = (qt_sample_ >= qt_half_window_size_)
        ? fixedpoint_to_size(qceil(qt_sample_ - qt_half_window_size_))
        : 0;
    roc_panic_if(ind_begin_cur > frame_size_ch_);
    ind_begin_cur = channelize_index(ind_begin_cur, channel_offset);

    ind_end_cur = ((qt_sample_ + qt_half_window_size_) > qt_frame_size_)
        ? frame_size_ch_ - 1
        : fixedpoint_to_size(qfloor(qt_sample_ + qt_half_window_size_));
    roc_panic_if(ind_end_cur > frame_size_ch_);
    ind_end_cur = channelize_index(ind_end_cur, channel_offset);

    ind_end_next = ((qt_sample_ + qt_half_window_size_) > qt_frame_size_)
        ? fixedpoint_to_size(qfloor(qt_sample_ + qt_half_window_size_ - qt_frame_size_))
            + 1
        : 0;
    roc_panic_if(ind_end_next > frame_size_ch_);
    ind_end_next = channelize_index(ind_end_next, channel_offset);

    // Counter inside window.
    // t_sinc = (t_sample - ceil( t_sample - window_len/cutoff*scale )) * sinc_step
    const long_fixedpoint_t qt_cur_ = qt_frame_size_ + qt_sample_
        - qceil(qt_frame_size_ + qt_sample_ - qt_half_window_size_);
    fixedpoint_t qt_sinc_cur =
        (fixedpoint_t)((qt_cur_ * (long_fixedpoint_t)qt_sinc_step_) >> FRACT_BIT_COUNT);

    // sinc_table defined in positive half-plane, so at the begining of the window
    // qt_sinc_cur starts decreasing and after we cross 0 it will be increasing
    // till the end of the window.
    fixedpoint_t qt_sinc_inc = qt_sinc_step_;

    // Compute fractional part of time position at the begining. It wont change during
    // the run.
    float f_sinc_cur_fract = fractional(qt_sinc_cur << window_interp_bits_);
    sample_t accumulator = 0;

    size_t i;

    // Run through previous frame.
    for (i = ind_begin_prev; i < ind_end_prev; i += channels_num_) {
        accumulator += prev_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur -= qt_sinc_inc;
    }

    // Run through current frame through the left windows side. qt_sinc_cur is decreasing.
    i = ind_begin_cur;

    accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
    while (qt_sinc_cur >= qt_sinc_step_) {
        i += channels_num_;
        qt_sinc_cur -= qt_sinc_inc;
        accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
    }

    i += channels_num_;

    roc_panic_if(i > channelize_index(frame_size_ch_, channel_offset));

    // Crossing zero -- we just need to switch qt_sinc_cur.
    // -1 ------------ 0 ------------- +1
    //      ^                  ^
    //      |                  |
    //   -qt_sinc_cur  ->  +qt_sinc_cur     <=> qt_sinc_cur = 1 - qt_sinc_cur
    qt_sinc_cur = qt_sinc_step_ - qt_sinc_cur; // qt_sinc_cur = -qt_sinc_cur + 1;
    f_sinc_cur_fract = fractional(qt_sinc_cur << window_interp_bits_);

    // Run through right side of the window, increasing qt_sinc_cur.
    for (; i <= ind_end_cur; i += channels_num_) {
        accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += qt_sinc_inc;
    }

    // Next frames run.
    for (i = ind_begin_next; i < ind_end_next; i += channels_num_) {
        accumulator += next_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += qt_sinc_inc;
    }

    return accumulator;
}

} // namespace audio
} // namespace roc
