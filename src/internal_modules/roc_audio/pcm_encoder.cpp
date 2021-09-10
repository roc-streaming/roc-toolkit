/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_encoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

PcmEncoder::PcmEncoder(const PcmFuncs& funcs)
    : funcs_(funcs)
    , frame_data_(NULL)
    , frame_size_(0)
    , frame_pos_(0) {
}

size_t PcmEncoder::encoded_size(size_t num_samples) const {
    return funcs_.payload_size_from_samples(num_samples);
}

void PcmEncoder::begin(void* frame_data, size_t frame_size) {
    roc_panic_if_not(frame_data);

    if (frame_data_) {
        roc_panic("pcm encoder: unpaired begin/end");
    }

    frame_data_ = frame_data;
    frame_size_ = frame_size;
}

size_t PcmEncoder::write(const audio::sample_t* samples,
                         size_t n_samples,
                         packet::channel_mask_t channels) {
    if (!frame_data_) {
        roc_panic("pcm encoder: write should be called only between begin/end");
    }

    const size_t wr_samples = funcs_.encode_samples(frame_data_, frame_size_, frame_pos_,
                                                    samples, n_samples, channels);

    frame_pos_ += wr_samples;
    return wr_samples;
}

void PcmEncoder::end() {
    if (!frame_data_) {
        roc_panic("pcm encoder: unpaired begin/end");
    }

    frame_data_ = NULL;
    frame_size_ = 0;
    frame_pos_ = 0;
}

} // namespace audio
} // namespace roc
