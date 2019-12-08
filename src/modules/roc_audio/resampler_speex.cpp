/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_speex.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_core/unique_ptr.h"

namespace roc {
namespace audio {

SpeexResampler::SpeexResampler(core::IAllocator& allocator,
                               const ResamplerConfig& config,
                               packet::channel_mask_t channels,
                               size_t frame_size)
    : allocator(allocator)
    , channel_mask_(channels)
    , channels_num_(packet::num_channels(channel_mask_))
    , speex_state(NULL)
    , prev_frame_(NULL)
    , curr_frame_(NULL)
    , next_frame_(NULL)
    , out_frame_pos_(0)
    , scaling_(1.0)
    , frame_size_(frame_size)
    , frame_size_ch_(channels_num_ ? frame_size / channels_num_ : 0)
    , counter(0) // delete me
    , valid_(false) {
    valid_ = true;
}

SpeexResampler::~SpeexResampler() {
    if (speex_state) {
        free(speex_state);
    }
}

bool SpeexResampler::valid() const {
    return valid_;
}

bool SpeexResampler::refresh_state() {
    int err_init;
    if (speex_state) {
        free(speex_state);
    }
    // speex_resampler_init does a C calloc(), SpeexResampler does not have a destructor,
    // then it can't be managed by a UniquePtr (it calls the destructor of the managed
    // object) maybe a solution would be creating a C++ wrapper for these calls and
    // SpeexResampler (having a destructor with 'free(speex_state)') ?
    speex_state =
        speex_resampler_init(channels_num_, input_sample_rate_ * sample_rate_multiplier_,
                             output_sample_rate_, 5, &err_init);
    return err_init == 0;
};

bool SpeexResampler::set_scaling(float input_sample_rate,
                                 float output_sample_rate,
                                 float multiplier) {
    input_sample_rate_ = input_sample_rate;
    output_sample_rate_ = output_sample_rate;
    sample_rate_multiplier_ = multiplier;
    refresh_state();
    return true;
}

bool SpeexResampler::resample_buff(Frame& out) {
    sample_t* out_data = out.data();
    sample_t* in_data = curr_frame_;

    spx_uint32_t in_len_val = frame_size_;
    spx_uint32_t out_len_val = out.size();

    int err = speex_resampler_process_interleaved_float(speex_state, in_data, &in_len_val,
                                                        out_data, &out_len_val);

    return err == 0;
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

} // namespace audio
} // namespace roc
