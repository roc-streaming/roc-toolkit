/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/panic.h"

#include "roc_audio/resampler.h"

// #define _USE_MATH_DEFINES
#include <math.h>
#include "roc_core/math.h"

namespace roc {
namespace audio {

using packet::sample_t;

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
const fixedpoint_t G_qt_one = 1 << FRACT_BIT_COUNT;

// Convert float to fixed-point.
inline fixedpoint_t float_to_fixedpoint(const float t) {
    return (fixedpoint_t)(t * (float)G_qt_one);
}

// Convert float to signed fixed-point.
inline signed_fixedpoint_t float_to_sfixedpoint(const float t) {
    return (signed_fixedpoint_t)(t * (float)G_qt_one);
}

inline size_t fixedpoint_to_size(const fixedpoint_t t) {
    return t >> FRACT_BIT_COUNT;
}

// Rounds x (Q8.24) upward.
inline fixedpoint_t qceil(const fixedpoint_t x) {
    if ((x & FRACT_PART_MASK) == 0) {
        return x & INTEGER_PART_MASK;
    } else {
        return (x & INTEGER_PART_MASK) + G_qt_one;
    }
}

// Rounds x (Q8.24) downward.
inline fixedpoint_t qfloor(const fixedpoint_t x) {
    // Just remove fractional part.
    return x & INTEGER_PART_MASK;
}

// Returns fractional part of x in f32.
inline float fractional(const fixedpoint_t x) {
    return (float)(x & FRACT_PART_MASK) * ((float)1. / (float)G_qt_one);
}

} // namespace

Resampler::Resampler(IStreamReader& reader,
                     ISampleBufferComposer& composer,
                     size_t frame_size)
    : reader_(reader)
    , window_(3)
    , scaling_(0)
    , frame_size_(frame_size)
    , window_len_(128)
    , qt_half_sinc_window_len_(float_to_fixedpoint(window_len_))
    , window_interp_(512)
    , window_interp_bits_(9)
    , sinc_table_(window_len_ * window_interp_ + 2)
    , qt_half_window_len_(float_to_fixedpoint((float)window_len_ / scaling_))
    , G_qt_epsilon_(float_to_fixedpoint(5e-8f))
    , G_default_sample_(float_to_fixedpoint(0))
    , qt_frame_size_(fixedpoint_t(frame_size_ << FRACT_BIT_COUNT))
    , qt_sample_(G_default_sample_)
    , qt_dt_(0)
    , cutoff_freq_(1.0f)
    {

    roc_panic_if(((fixedpoint_t)-1 >> FRACT_BIT_COUNT) < frame_size_);
    init_window_(composer);
    fill_sinc();

    roc_panic_if_not(set_scaling(1.0f));
}

bool Resampler::set_scaling(float scaling) {
    // Window's size changes according to scaling. If new window size
    // doesnt fit to the frames size -- deny changes.
    if (window_len_ * scaling >= frame_size_) {
        return false;
    }
    scaling_ = scaling;
    // In case of upscaling one should properly shift the edge frequency
    // of the digital filter.
    // In both cases it's sensible to decrease the edge frequency to leave
    // some.
    if (scaling_ > 1.0f) {
        qt_sinc_step_ = float_to_sfixedpoint(cutoff_freq_/scaling_);
        qt_half_window_len_ = float_to_fixedpoint((float)window_len_ / cutoff_freq_ * scaling_);
    } else {
        qt_sinc_step_ = float_to_sfixedpoint(cutoff_freq_);
        qt_half_window_len_ = float_to_fixedpoint((float)window_len_ / cutoff_freq_);
    }
    qt_half_sinc_window_len_ = float_to_fixedpoint(window_len_);
    return true;
}

void Resampler::read(const ISampleBufferSlice& buff) {
    sample_t* buff_data = buff.data();
    roc_panic_if(buff_data == NULL);

    size_t buff_size = buff.size();

    if (curr_frame_ == NULL) {
        qt_sample_ = G_default_sample_;
        renew_window_();
    }

    for (size_t n = 0; n < buff_size; n++) {
        if (qt_sample_ >= qt_frame_size_) {
            qt_sample_ -= qt_frame_size_;
            renew_window_();
        }

        buff_data[n] = resample_();
        qt_sample_ += qt_dt_;
    }
}

void Resampler::init_window_(ISampleBufferComposer& composer) {
    roc_log(LogDebug, "resampler: initializing window");

    for (size_t n = 0; n < window_.size(); n++) {
        if (!(window_[n] = composer.compose())) {
            roc_panic("resampler: can't compose buffer in constructor");
        }
        window_[n]->set_size(frame_size_);
    }

    prev_frame_ = NULL;
    curr_frame_ = NULL;
    next_frame_ = NULL;
}

void Resampler::renew_window_() {
    roc_panic_if(window_len_ * scaling_ >= frame_size_);

    // scaling_ may change every frame so it have to be smooth.
    qt_dt_ = float_to_fixedpoint(scaling_);

    if (curr_frame_ == NULL) {
        reader_.read(*window_[0]);
        reader_.read(*window_[1]);
        reader_.read(*window_[2]);
    } else {
        window_.rotate(1);
        reader_.read(*window_.back());
        roc_panic_if(window_.back()->size() != frame_size_);
    }

    prev_frame_ = window_[0]->data();
    curr_frame_ = window_[1]->data();
    next_frame_ = window_[2]->data();
}

void Resampler::fill_sinc() {
    const sample_t sinc_step = 1.0f/(sample_t)window_interp_;
    sample_t sinc_t = sinc_step;
    sinc_table_[0] = 1.0f;
    for (size_t i = 1; i < sinc_table_.size(); ++i) {
        sinc_table_[i] = (float)(sin(M_PI * (double)sinc_t) / M_PI) / sinc_t;
        sinc_t += sinc_step;
    }
    sinc_table_[sinc_table_.size()-2] = 0;
    sinc_table_[sinc_table_.size()-1] = 0;
}

// Computes sinc value in x position using linear interpolation between
// table values from sinc_table.h
//
// During going through input signal window only integer part of argument changes,
// that's why there are two arguments in this function: integer part and fractional
// part of time coordinate.
sample_t Resampler::sinc_(const fixedpoint_t x, const float fract_x) {
    roc_panic_if(x > (window_len_ << FRACT_BIT_COUNT));

    // Tables index smaller than to x
    const sample_t hl = sinc_table_[(x >> (FRACT_BIT_COUNT - window_interp_bits_))];

    // Tables index next to x
    const sample_t hh = sinc_table_[(x >> (FRACT_BIT_COUNT - window_interp_bits_)) + 1];

    return hl + fract_x * (hh - hl);
}

sample_t Resampler::resample_() {
    // Index of first input sample in window.
    size_t ind_begin_prev;

    // Window lasts till that index.
    const size_t ind_end_prev = frame_size_;

    size_t ind_begin_cur;
    size_t ind_end_cur;

    const size_t ind_begin_next = 0;
    size_t ind_end_next;

    if ((qt_sample_ & FRACT_PART_MASK) < G_qt_epsilon_) {
        qt_sample_ &= INTEGER_PART_MASK;
    } else if ((G_qt_one - (qt_sample_ & FRACT_PART_MASK)) < G_qt_epsilon_) {
        qt_sample_ &= INTEGER_PART_MASK;
        qt_sample_ += G_qt_one;
    }

    ind_begin_prev = (qt_sample_ >= qt_half_window_len_)
        ? frame_size_
        : fixedpoint_to_size(qceil(qt_sample_ + (qt_frame_size_ - qt_half_window_len_)));
    roc_panic_if(ind_begin_prev > frame_size_);

    ind_begin_cur = (qt_sample_ >= qt_half_window_len_)
        ? fixedpoint_to_size(qceil(qt_sample_ - qt_half_window_len_))
        : 0;
    roc_panic_if(ind_begin_cur > frame_size_);

    ind_end_cur = ((qt_sample_ + qt_half_window_len_) > qt_frame_size_)
        ? frame_size_
        : fixedpoint_to_size(qfloor(qt_sample_ + qt_half_window_len_));
    roc_panic_if(ind_end_cur > frame_size_);

    ind_end_next = ((qt_sample_ + qt_half_window_len_) > qt_frame_size_)
        ? fixedpoint_to_size(qfloor(qt_sample_ + qt_half_window_len_ - qt_frame_size_))
        : 0;
    roc_panic_if(ind_end_next > frame_size_);

    // Counter inside window.
    // t_sinc = t_sample - ceil( t_sample - window_len_ + 1/window_interp_ )
    fixedpoint_t qt_sinc_cur = qt_frame_size_ + qt_sample_
        - qceil(qt_frame_size_ + qt_sample_ - qt_half_sinc_window_len_);

    // sinc_table defined in positive half-plane, so at the begining of the window
    // qt_sinc_cur starts decreasing and after we cross 0 it will be increasing
    // till the end of the window.
    signed_fixedpoint_t qt_sinc_inc = -qt_sinc_step_;

    // Compute fractional part of time position at the begining. It wont change during
    // the run.
    float f_sinc_cur_fract = fractional(qt_sinc_cur << window_interp_bits_);
    sample_t accumulator = 0;

    size_t i;

    // Run through previous frame.
    for (i = ind_begin_prev; i < ind_end_prev; ++i) {
        accumulator += prev_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += (fixedpoint_t)qt_sinc_inc;
    }

    // Run through current frame through the left windows side. qt_sinc_cur is decreasing.
    i = ind_begin_cur;

    accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
    while (qt_sinc_cur >= G_qt_one) {
        ++i;
        qt_sinc_cur += (fixedpoint_t)qt_sinc_inc;
        accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
    }

    ++i;

    roc_panic_if(i > frame_size_);
    // Run through right side of the window, increasing qt_sinc_cur.
    qt_sinc_inc = -qt_sinc_inc;

    // Crossing zero -- we just need to switch qt_sinc_cur.
    // -1 ------------ 0 ------------- +1
    //      ^                  ^
    //      |                  |
    //   -qt_sinc_cur  ->  +qt_sinc_cur     <=> qt_sinc_cur = 1 - qt_sinc_cur
    qt_sinc_cur = G_qt_one - qt_sinc_cur; // qt_sinc_cur = -qt_sinc_cur + 1;
    f_sinc_cur_fract = fractional(qt_sinc_cur << window_interp_bits_);

    for (; i < ind_end_cur; ++i) {
        accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += (fixedpoint_t)qt_sinc_inc;
    }

    // Next frames run.
    for (i = ind_begin_next; i < ind_end_next; ++i) {
        accumulator += next_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += (fixedpoint_t)qt_sinc_inc;
    }

    return accumulator;
}

} // namespace audio
} // namespace roc
