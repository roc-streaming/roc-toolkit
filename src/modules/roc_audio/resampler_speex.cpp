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

#include <iostream>

namespace roc {
namespace audio {

SpeexResampler::SpeexResampler(core::IAllocator& allocator,
                               const ResamplerConfig& config,
                               packet::channel_mask_t channels,
                               size_t frame_size)
    : channel_mask_(channels)
    , channels_num_(packet::num_channels(channel_mask_))
    , speex_state(NULL)
    , prev_frame_(NULL)
    , curr_frame_(NULL)
    , next_frame_(NULL)
    , out_frame_pos_(0)
    , in_offset(0)
    , scaling_(1.0)
    , frame_size_(frame_size)
    , frame_size_ch_(channels_num_ ? frame_size / channels_num_ : 0)
    , valid_(false) {
        if(!check_config_()){
            return;
        }
        
        valid_ = true;
}

SpeexResampler::~SpeexResampler() {
    if (speex_state) {
        speex_resampler_destroy(speex_state);
    }
}

bool SpeexResampler::valid() const {
    return valid_;
}

bool SpeexResampler::refresh_state() {
    int err_init;
    if (speex_state) {
        speex_resampler_destroy(speex_state);
    }

    speex_state =
        speex_resampler_init(1, input_sample_rate_ * sample_rate_multiplier_,
                             output_sample_rate_, 5, &err_init);
    return err_init == 0;
};

bool SpeexResampler::set_scaling(float input_sample_rate,
                                 float output_sample_rate,
                                 float multiplier) {
    input_sample_rate_ = input_sample_rate;
    output_sample_rate_ = output_sample_rate;
    sample_rate_multiplier_ = multiplier;
    
    return refresh_state();
}

bool SpeexResampler::resample_buff(Frame& out) {
    roc_panic_if(!prev_frame_);
    roc_panic_if(!curr_frame_);
    roc_panic_if(!next_frame_);

    sample_t* out_data = out.data();
    sample_t* in_data = curr_frame_;

    spx_uint32_t in_len_val = frame_size_;
    spx_uint32_t out_len_val = out.size() - out_frame_pos_;

    spx_uint32_t remaining_in = in_len_val;
    spx_uint32_t remaining_out = out_len_val;

    while(out_frame_pos_ < out_len_val && remaining_out > 0){
        // remaining_in after this call is the number of input processed samples, the same happens to remaining_out
        int err = speex_resampler_process_interleaved_float(speex_state, in_data + in_offset,  &remaining_in, out_data + out_frame_pos_, &remaining_out);
        roc_panic_if(err);

        in_offset += remaining_in;
        out_frame_pos_ += remaining_out;
        //std::cout << "rem in " << remaining_in << std::endl;

        remaining_in = in_len_val > in_offset ? in_len_val - in_offset : 0;
        remaining_out = out_len_val > out_frame_pos_ ? out_len_val - out_frame_pos_ : 0;
        /*
        std::cout << "remaining_in = " << remaining_in << ", remaining_out = " << remaining_out 
        << ", out_frame_pos_ = " << out_frame_pos_ << ", in_offset = " << in_offset << ", out_len_val = " << out_len_val
        << ", in_len_val = " << in_len_val  << std::endl;
        */
        /*
        if(in_offset > in_len_val || out_frame_pos_ > out_len_val){
            roc_panic("speex wrote out of bounds mem!!");
        }*/

        if(remaining_in == 0){
            //std::cout << "FALSE" << std::endl;
            in_offset = 0;
            return false;
        }
    }
    out_frame_pos_ = 0;
    //std::cout << "TRUE" << std::endl;
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
}

bool SpeexResampler::check_config_() const {
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

    return true;
}


} // namespace audio
} // namespace roc
