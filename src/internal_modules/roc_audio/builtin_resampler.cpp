/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/builtin_resampler.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

// Fixed point type Q8.24 for realizing computations of curr_frame_ in fixed point
// arithmetic. Sometimes this computations requires ceil(...) and floor(...) and
// it is very CPU-time hungry in floating point variant on x86.
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

// Convert float to fixed-point.
inline float fixedpoint_to_float(const fixedpoint_t f) {
    return f / (float)qt_one;
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

inline size_t get_window_interp(ResamplerProfile profile) {
    switch (profile) {
    case ResamplerProfile_Low:
        return 64;

    case ResamplerProfile_Medium:
        return 128;

    case ResamplerProfile_High:
        return 512;
    }

    roc_panic("builtin resampler: unexpected profile");
}

inline size_t get_window_size(ResamplerProfile profile) {
    switch (profile) {
    case ResamplerProfile_Low:
        return 16;

    case ResamplerProfile_Medium:
        return 32;

    case ResamplerProfile_High:
        return 64;
    }

    roc_panic("builtin resampler: unexpected profile");
}

inline size_t get_frame_size(size_t window_size,
                             const SampleSpec& in_spec,
                             const SampleSpec& out_spec) {
    const float scaling =
        (float)in_spec.sample_rate() / (float)out_spec.sample_rate() * 1.5f;

    return (size_t)std::ceil(window_size * scaling);
}

} // namespace

BuiltinResampler::BuiltinResampler(const ResamplerConfig& config,
                                   const SampleSpec& in_spec,
                                   const SampleSpec& out_spec,
                                   FrameFactory& frame_factory,
                                   core::IArena& arena)
    : IResampler(arena)
    , in_spec_(in_spec)
    , out_spec_(out_spec)
    , n_ready_frames_(0)
    , prev_frame_(NULL)
    , curr_frame_(NULL)
    , next_frame_(NULL)
    , scaling_(1.0)
    , window_size_(get_window_size(config.profile))
    , qt_half_sinc_window_size_(float_to_fixedpoint(window_size_))
    , window_interp_(get_window_interp(config.profile))
    , window_interp_bits_(calc_bits(window_interp_))
    , frame_size_ch_(get_frame_size(window_size_, in_spec, out_spec))
    , frame_size_(frame_size_ch_ * in_spec.num_channels())
    , sinc_table_(arena)
    , sinc_table_ptr_(NULL)
    , qt_half_window_size_(float_to_fixedpoint((float)window_size_ / scaling_))
    , qt_epsilon_(float_to_fixedpoint(5e-8f))
    , qt_frame_size_(fixedpoint_t(frame_size_ch_ << FRACT_BIT_COUNT))
    , qt_sample_(float_to_fixedpoint(0))
    , qt_dt_(0)
    , cutoff_freq_(0.9f)
    , init_status_(status::NoStatus) {
    if (!in_spec_.is_complete() || !out_spec_.is_complete() || !in_spec_.is_raw()
        || !out_spec_.is_raw()) {
        roc_panic("builtin resampler: required complete sample specs with raw format:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    if (in_spec_.channel_set() != out_spec_.channel_set()) {
        roc_panic("builtin resampler: required identical input and output channel sets:"
                  " in_spec=%s out_spec=%s",
                  sample_spec_to_str(in_spec_).c_str(),
                  sample_spec_to_str(out_spec_).c_str());
    }

    roc_log(
        LogDebug,
        "builtin resampler: initializing:"
        " profile=%s window_interp=%lu window_size=%lu frame_size=%lu channels_num=%lu",
        resampler_profile_to_str(config.profile), (unsigned long)window_interp_,
        (unsigned long)window_size_, (unsigned long)frame_size_,
        (unsigned long)in_spec_.num_channels());

    if (!check_config_()) {
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (!fill_sinc_()) {
        init_status_ = status::StatusNoMem;
        return;
    }

    if (!alloc_frames_(frame_factory)) {
        init_status_ = status::StatusNoMem;
        return;
    }

    init_status_ = status::StatusOK;
}

BuiltinResampler::~BuiltinResampler() {
}

status::StatusCode BuiltinResampler::init_status() const {
    return init_status_;
}

bool BuiltinResampler::set_scaling(size_t input_sample_rate,
                                   size_t output_sample_rate,
                                   float multiplier) {
    if (input_sample_rate == 0 || output_sample_rate == 0) {
        roc_log(LogError, "builtin resampler: invalid rate");
        return false;
    }

    const float new_scaling = float(input_sample_rate) / output_sample_rate * multiplier;

    // Filter out obviously invalid values.
    if (new_scaling <= 0) {
        roc_log(LogError, "builtin resampler: invalid scaling");
        return false;
    }

    // Window's size changes according to scaling. If new window size
    // doesn't fit to the frames size -- deny changes.
    if (window_size_ * new_scaling > frame_size_ch_ - 1) {
        roc_log(LogError,
                "builtin resampler: scaling does not fit frame size:"
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
                    "builtin resampler: scaling does not fit window size:"
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
    qt_dt_ = float_to_fixedpoint(scaling_);

    return true;
}

const core::Slice<sample_t>& BuiltinResampler::begin_push_input() {
    if (n_ready_frames_ < 3) {
        return frames_[n_ready_frames_];
    }

    core::Slice<sample_t> new_last_frame = frames_[0];
    frames_[0] = frames_[1];
    frames_[1] = frames_[2];
    frames_[2] = new_last_frame;

    return frames_[2];
}

void BuiltinResampler::end_push_input() {
    prev_frame_ = frames_[0].data();
    curr_frame_ = frames_[1].data();
    next_frame_ = frames_[2].data();

    if (n_ready_frames_ < 3) {
        n_ready_frames_++;
    }

    if (qt_sample_ >= qt_frame_size_) {
        qt_sample_ -= qt_frame_size_;
    }
}

size_t BuiltinResampler::pop_output(sample_t* out_data, size_t out_size) {
    if (n_ready_frames_ < 3) {
        return 0;
    }

    size_t out_pos = 0;

    for (; out_pos < out_size; out_pos += in_spec_.num_channels()) {
        if (qt_sample_ >= qt_frame_size_) {
            break;
        }

        if ((qt_sample_ & FRACT_PART_MASK) < qt_epsilon_) {
            qt_sample_ &= INTEGER_PART_MASK;
        } else if ((qt_one - (qt_sample_ & FRACT_PART_MASK)) < qt_epsilon_) {
            qt_sample_ &= INTEGER_PART_MASK;
            qt_sample_ += qt_one;
        }

        for (size_t channel = 0; channel < in_spec_.num_channels(); ++channel) {
            out_data[out_pos + channel] = resample_(channel);
        }
        qt_sample_ += qt_dt_;
    }

    return out_pos;
}

float BuiltinResampler::n_left_to_process() const {
    return fixedpoint_to_float(2 * qt_frame_size_ - qt_sample_) * in_spec_.num_channels();
}

bool BuiltinResampler::alloc_frames_(FrameFactory& frame_factory) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(frames_); n++) {
        frames_[n] = frame_factory.new_raw_buffer();

        if (!frames_[n]) {
            roc_log(LogError, "builtin resampler: can't allocate frame buffer");
            return false;
        }

        frames_[n].reslice(0, frame_size_);
    }

    return true;
}

bool BuiltinResampler::check_config_() const {
    if (frame_size_ != frame_size_ch_ * in_spec_.num_channels()) {
        roc_log(LogError,
                "builtin resampler: frame_size is not multiple of num_channels:"
                " frame_size=%lu num_channels=%lu",
                (unsigned long)frame_size_, (unsigned long)in_spec_.num_channels());
        return false;
    }

    const size_t max_frame_size =
        (((fixedpoint_t)(signed_fixedpoint_t)-1 >> FRACT_BIT_COUNT) + 1)
        * in_spec_.num_channels();

    if (frame_size_ > max_frame_size) {
        roc_log(LogError,
                "builtin resampler: frame_size is too much:"
                " max_frame_size=%lu frame_size=%lu num_channels=%lu",
                (unsigned long)max_frame_size, (unsigned long)frame_size_,
                (unsigned long)in_spec_.num_channels());
        return false;
    }

    if ((size_t)1 << window_interp_bits_ != window_interp_) {
        roc_log(LogError,
                "builtin resampler: window_interp is not power of two:"
                " window_interp=%lu",
                (unsigned long)window_interp_);
        return false;
    }

    return true;
}

bool BuiltinResampler::fill_sinc_() {
    if (!sinc_table_.resize(window_size_ * window_interp_ + 2)) {
        roc_log(LogError, "builtin resampler: can't allocate sinc table");
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
    roc_panic_if_msg(qt_sinc_step_ == 0,
                     "builtin resampler:"
                     " set_scaling() must be called before any resampling could be done");
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

    // sinc_table defined in positive half-plane, so at the beginning of the window
    // qt_sinc_cur starts decreasing and after we cross 0 it will be increasing
    // till the end of the window.
    fixedpoint_t qt_sinc_inc = qt_sinc_step_;

    // Compute fractional part of time position at the beginning. It wont change during
    // the run.
    float f_sinc_cur_fract = fractional(qt_sinc_cur << window_interp_bits_);
    sample_t accumulator = 0;

    size_t i;

    // Run through previous frame.
    for (i = ind_begin_prev; i < ind_end_prev; i += in_spec_.num_channels()) {
        accumulator += prev_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur -= qt_sinc_inc;
    }

    // Run through current frame through the left windows side. qt_sinc_cur is decreasing.
    i = ind_begin_cur;

    accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
    while (qt_sinc_cur >= qt_sinc_step_) {
        i += in_spec_.num_channels();
        qt_sinc_cur -= qt_sinc_inc;
        accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
    }

    i += in_spec_.num_channels();

    roc_panic_if(i > channelize_index(frame_size_ch_, channel_offset));

    // Crossing zero -- we just need to switch qt_sinc_cur.
    // -1 ------------ 0 ------------- +1
    //      ^                  ^
    //      |                  |
    //   -qt_sinc_cur  ->  +qt_sinc_cur     <=> qt_sinc_cur = 1 - qt_sinc_cur
    qt_sinc_cur = qt_sinc_step_ - qt_sinc_cur; // qt_sinc_cur = -qt_sinc_cur + 1;
    f_sinc_cur_fract = fractional(qt_sinc_cur << window_interp_bits_);

    // Run through right side of the window, increasing qt_sinc_cur.
    for (; i <= ind_end_cur; i += in_spec_.num_channels()) {
        accumulator += curr_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += qt_sinc_inc;
    }

    // Next frames run.
    for (i = ind_begin_next; i < ind_end_next; i += in_spec_.num_channels()) {
        accumulator += next_frame_[i] * sinc_(qt_sinc_cur, f_sinc_cur_fract);
        qt_sinc_cur += qt_sinc_inc;
    }

    return accumulator;
}

} // namespace audio
} // namespace roc
