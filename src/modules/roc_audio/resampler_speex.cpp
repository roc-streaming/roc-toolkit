/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_speex.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

inline int get_quality(ResamplerProfile profile) {
    switch (profile) {
    case ResamplerProfile_Low:
        return 1;

    case ResamplerProfile_Medium:
        return 5;

    case ResamplerProfile_High:
        return 10;
    }
}

} // namespace

SpeexResampler::SpeexResampler(core::IAllocator& allocator,
                               ResamplerProfile profile,
                               core::nanoseconds_t frame_length,
                               size_t sample_rate,
                               packet::channel_mask_t channels)
    : speex_state_(NULL)
    , prev_frame_(NULL)
    , curr_frame_(NULL)
    , next_frame_(NULL)
    , mix_frame_(allocator)
    , out_frame_pos_(0)
    , in_offset_(0)
    , frame_size_((spx_uint32_t)packet::ns_to_size(frame_length, sample_rate, channels))
    , quality_(get_quality(profile))
    , valid_(false) {
    if (frame_size_ == 0) {
        return;
    }

    if (!mix_frame_.resize(frame_size_ * 3)) {
        return;
    }

    valid_ = true;
}

SpeexResampler::~SpeexResampler() {
    if (speex_state_) {
        speex_resampler_destroy(speex_state_);
    }
}

bool SpeexResampler::valid() const {
    return valid_;
}

bool SpeexResampler::refresh_state_() {
    if (speex_state_) {
        speex_resampler_destroy(speex_state_);
    }

    int err_init = 0;
    speex_state_ = speex_resampler_init(
        1, spx_uint32_t(input_sample_rate_ * sample_rate_multiplier_),
        output_sample_rate_, quality_, &err_init);

    return err_init == 0;
}

bool SpeexResampler::set_scaling(size_t input_sample_rate,
                                 size_t output_sample_rate,
                                 float multiplier) {
    input_sample_rate_ = (spx_uint32_t)input_sample_rate;
    output_sample_rate_ = (spx_uint32_t)output_sample_rate;
    sample_rate_multiplier_ = multiplier;

    return refresh_state_();
}

bool SpeexResampler::resample_buff(Frame& out) {
    roc_panic_if(!prev_frame_);
    roc_panic_if(!curr_frame_);
    roc_panic_if(!next_frame_);

    sample_t* out_data = out.data();
    sample_t* in_data = mix_frame_.data() + frame_size_;

    spx_uint32_t in_len_val = frame_size_;
    spx_uint32_t out_len_val = (spx_uint32_t)out.size() - out_frame_pos_;

    spx_uint32_t remaining_in = in_len_val;
    spx_uint32_t remaining_out = out_len_val;

    while (out_frame_pos_ < out_len_val) {
        // remaining_in after this call is the number of input processed samples, the same
        // happens to remaining_out
        const int err = speex_resampler_process_float(
            speex_state_, 0, in_data + in_offset_, &remaining_in,
            out_data + out_frame_pos_, &remaining_out);

        roc_panic_if(err);

        in_offset_ += remaining_in;
        out_frame_pos_ += remaining_out;

        remaining_in = in_len_val > in_offset_ ? in_len_val - in_offset_ : 0;
        remaining_out = out_len_val > out_frame_pos_ ? out_len_val - out_frame_pos_ : 0;

        if (remaining_in == 0) {
            in_offset_ = 0;
            return false;
        }
    }

    out_frame_pos_ = 0;
    return true;
}

void SpeexResampler::renew_buffers(core::Slice<sample_t>& prev,
                                   core::Slice<sample_t>& cur,
                                   core::Slice<sample_t>& next) {
    roc_panic_if(prev.size() != frame_size_);
    roc_panic_if(cur.size() != frame_size_);
    roc_panic_if(next.size() != frame_size_);

    prev_frame_ = prev.data();
    curr_frame_ = cur.data();
    next_frame_ = next.data();

    memcpy(mix_frame_.data(), prev_frame_, frame_size_ * sizeof(sample_t));
    memcpy(mix_frame_.data() + frame_size_, curr_frame_, frame_size_ * sizeof(sample_t));
    memcpy(mix_frame_.data() + frame_size_ * 2, next_frame_,
           frame_size_ * sizeof(sample_t));
}

} // namespace audio
} // namespace roc
