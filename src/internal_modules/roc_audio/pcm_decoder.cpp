/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_decoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

PcmDecoder::PcmDecoder(const PcmFuncs& funcs)
    : funcs_(funcs)
    , stream_pos_(0)
    , stream_avail_(0)
    , frame_data_(NULL)
    , frame_size_(0)
    , frame_pos_(0) {
}

packet::timestamp_t PcmDecoder::position() const {
    return stream_pos_;
}

packet::timestamp_t PcmDecoder::available() const {
    return stream_avail_;
}

void PcmDecoder::begin(packet::timestamp_t frame_position,
                       const void* frame_data,
                       size_t frame_size) {
    roc_panic_if_not(frame_data);

    if (frame_data_) {
        roc_panic("pcm decoder: unpaired begin/end");
    }

    stream_pos_ = frame_position;
    stream_avail_ = (packet::timestamp_t)funcs_.samples_from_payload_size(frame_size);

    frame_data_ = frame_data;
    frame_size_ = frame_size;
}

size_t PcmDecoder::read(audio::sample_t* samples,
                        size_t n_samples,
                        packet::channel_mask_t channels) {
    if (!frame_data_) {
        roc_panic("pcm decoder: read should be called only between begin/end");
    }

    if (n_samples > (size_t)stream_avail_) {
        n_samples = (size_t)stream_avail_;
    }

    const size_t rd_samples = funcs_.decode_samples(frame_data_, frame_size_, frame_pos_,
                                                    samples, n_samples, channels);

    (void)shift(rd_samples);

    return rd_samples;
}

size_t PcmDecoder::shift(size_t n_samples) {
    if (!frame_data_) {
        roc_panic("pcm decoder: shift should be called only between begin/end");
    }

    if (n_samples > (size_t)stream_avail_) {
        n_samples = (size_t)stream_avail_;
    }

    stream_pos_ += (packet::timestamp_t)n_samples;
    stream_avail_ -= (packet::timestamp_t)n_samples;

    frame_pos_ += n_samples;

    return n_samples;
}

void PcmDecoder::end() {
    if (!frame_data_) {
        roc_panic("pcm decoder: unpaired begin/end");
    }

    stream_avail_ = 0;

    frame_data_ = NULL;
    frame_size_ = 0;
    frame_pos_ = 0;
}

} // namespace audio
} // namespace roc
