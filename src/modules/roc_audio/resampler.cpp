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
#include "roc_audio/sinc_table.h"

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

const uint32_t INTEGER_PART_MASK = 0xFF000000;
const uint32_t FRACT_PART_MASK = 0x00FFFFFF;
const uint32_t FRACT_BIT_COUNT = 24;
const uint32_t FRACT_BIT_TO_INDEX = 6; // Number of bits in st_Nwindow_interp.

// One in terms of Q8.24.
const fixedpoint_t G_qt_one = 1 << FRACT_BIT_COUNT;

// Convert float to fixed-point.
inline fixedpoint_t float_to_fixedpoint(const float t) {
    return (fixedpoint_t)(t * (float)G_qt_one);
}

inline size_t fixedpoint_to_size(const fixedpoint_t t) {
    return (t >> FRACT_BIT_COUNT) & 0xFF;
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

// Computes sinc value in x position using linear interpolation between
// table values from sinc_table.h
//
// During going through input signal window only integer part of argument changes,
// that's why there are two arguments in this function: integer part and fractional
// part of time coordinate.
inline sample_t sinc(const fixedpoint_t x, const float fract_x) {
    roc_panic_if(x > (st_Nwindow << FRACT_BIT_COUNT));

    // Tables index smaller than to x
    const sample_t hl = sinc_table[(x >> (FRACT_BIT_COUNT - FRACT_BIT_TO_INDEX))];

    // Tables index next to x
    const sample_t hh = sinc_table[(x >> (FRACT_BIT_COUNT - FRACT_BIT_TO_INDEX)) + 1];

    return hl + fract_x * (hh - hl);
}

//! How many input samples fits into length of half window.
const float G_ft_half_window_len = ((float)st_Nwindow);

// G_ft_half_window_len in Q8.24.
const fixedpoint_t G_qt_half_window_len = float_to_fixedpoint(G_ft_half_window_len);

const fixedpoint_t G_qt_epsilon = float_to_fixedpoint(5e-8f);

const fixedpoint_t G_default_sample = float_to_fixedpoint(0);

} // namespace

Resampler::Resampler(IStreamReader& reader,
                     ISampleBufferComposer& composer,
                     size_t frame_size)
    : reader_(reader)
    , window_(3)
    , frame_size_(frame_size)
    , qt_frame_size_(fixedpoint_t(frame_size_ << FRACT_BIT_COUNT))
    , qt_sample_(G_default_sample)
    , qt_dt_(0)
    , scaling_(0) {
    if (G_qt_half_window_len >= qt_frame_size_)
        roc_panic("Half window of resamplers IR must fit into one frame.");

    if ((uint64_t)qt_frame_size_ + (uint64_t)G_qt_half_window_len
        >= (INTEGER_PART_MASK + FRACT_PART_MASK))
        roc_panic("frame_size_ doesn't fit to integral part of fixedpoint type.");

    init_window_(composer);

    roc_panic_if_not(set_scaling(1.0f));
}

bool Resampler::set_scaling(float scaling) {
    // Window's size changes according to scaling. If new window size
    // doesnt fit to the frames size -- deny changes.
    if (st_Nwindow * scaling >= frame_size_) {
        return false;
    }
    scaling_ = scaling;
    return true;
}

void Resampler::read(const ISampleBufferSlice& buff) {
    sample_t* buff_data = buff.data();
    roc_panic_if(buff_data == NULL);

    size_t buff_size = buff.size();

    if (curr_frame_ == NULL) {
        qt_sample_ = G_default_sample;
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
    roc_panic_if(st_Nwindow * scaling_ >= frame_size_);

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

sample_t Resampler::resample_() {
    // Index of first input sample in window.
    size_t ind_begin_prev;

    // Window lasts till that index.
    const size_t ind_end_prev = frame_size_;

    size_t ind_begin_cur;
    size_t ind_end_cur;

    const size_t ind_begin_next = 0;
    size_t ind_end_next;

    if ((qt_sample_ & FRACT_PART_MASK) < G_qt_epsilon) {
        qt_sample_ &= INTEGER_PART_MASK;
    } else if ((G_qt_one - (qt_sample_ & FRACT_PART_MASK)) < G_qt_epsilon) {
        qt_sample_ &= INTEGER_PART_MASK;
        qt_sample_ += G_qt_one;
    }

    ind_begin_prev = (qt_sample_ >= G_qt_half_window_len)
        ? frame_size_
        : fixedpoint_to_size(qceil(qt_sample_ + (qt_frame_size_ - G_qt_half_window_len)));
    roc_panic_if(ind_begin_prev > frame_size_);

    ind_begin_cur = (qt_sample_ >= G_qt_half_window_len)
        ? fixedpoint_to_size(qceil(qt_sample_ - G_qt_half_window_len))
        : 0;
    roc_panic_if(ind_begin_cur > frame_size_);

    ind_end_cur = ((qt_sample_ + G_qt_half_window_len) > qt_frame_size_)
        ? frame_size_
        : fixedpoint_to_size(qfloor(qt_sample_ + G_qt_half_window_len));
    roc_panic_if(ind_end_cur > frame_size_);

    ind_end_next = ((qt_sample_ + G_qt_half_window_len) > qt_frame_size_)
        ? fixedpoint_to_size(qfloor(qt_sample_ + G_qt_half_window_len - qt_frame_size_))
        : 0;
    roc_panic_if(ind_end_next > frame_size_);

    // Counter inside window.
    // t_sinc = t_sample - ceil( t_sample - st_Nwindow + 1/st_Nwindow_interp )
    fixedpoint_t qt_sinc_cur = qt_frame_size_ + qt_sample_
        - qceil(qt_frame_size_ + qt_sample_ - G_qt_half_window_len);

    // sinc_table defined in positive half-plane, so at the begining of the window
    // qt_sinc_cur starts decreasing and after we cross 0 it will be increasing
    // till the end of the window.
    signed_fixedpoint_t qt_sinc_inc = -((signed_fixedpoint_t)G_qt_one);

    // Compute fractional part of time position at the begining. It wont change during
    // the run.
    float f_sinc_cur_fract = fractional(qt_sinc_cur << FRACT_BIT_TO_INDEX);
    sample_t accumulator = 0;

    size_t i;

    // Run through previous frame.
    for (i = ind_begin_prev; i < ind_end_prev; ++i) {
        accumulator += prev_frame_[i] * sinc(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += (fixedpoint_t)qt_sinc_inc;
    }

    // Run through current frame through the left windows side. qt_sinc_cur is decreasing.
    i = ind_begin_cur;

    accumulator += curr_frame_[i] * sinc(qt_sinc_cur, f_sinc_cur_fract);
    while (qt_sinc_cur >= G_qt_one) {
        ++i;
        qt_sinc_cur += (fixedpoint_t)qt_sinc_inc;
        accumulator += curr_frame_[i] * sinc(qt_sinc_cur, f_sinc_cur_fract);
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
    f_sinc_cur_fract = fractional(qt_sinc_cur << FRACT_BIT_TO_INDEX);

    for (; i < ind_end_cur; ++i) {
        accumulator += curr_frame_[i] * sinc(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += (fixedpoint_t)qt_sinc_inc;
    }

    // Next frames run.
    for (i = ind_begin_next; i < ind_end_next; ++i) {
        accumulator += next_frame_[i] * sinc(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += (fixedpoint_t)qt_sinc_inc;
    }

    return accumulator;
}

} // namespace audio
} // namespace roc
