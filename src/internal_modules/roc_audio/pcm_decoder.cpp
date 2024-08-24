/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_decoder.h"
#include "roc_audio/sample.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

IFrameDecoder* PcmDecoder::construct(const SampleSpec& sample_spec, core::IArena& arena) {
    return new (arena) PcmDecoder(sample_spec, arena);
}

PcmDecoder::PcmDecoder(const SampleSpec& sample_spec, core::IArena& arena)
    : IFrameDecoder(arena)
    , pcm_mapper_(sample_spec.pcm_subformat(), PcmSubformat_Raw)
    , n_chans_(sample_spec.num_channels())
    , stream_pos_(0)
    , stream_avail_(0)
    , frame_data_(NULL)
    , frame_byte_size_(0)
    , frame_bit_off_(0) {
}

status::StatusCode PcmDecoder::init_status() const {
    return status::StatusOK;
}

packet::stream_timestamp_t PcmDecoder::position() const {
    return stream_pos_;
}

packet::stream_timestamp_t PcmDecoder::available() const {
    return stream_avail_;
}

size_t PcmDecoder::decoded_sample_count(const void* frame_data, size_t frame_size) const {
    roc_panic_if_not(frame_data);

    return pcm_mapper_.input_sample_count(frame_size) / n_chans_;
}

void PcmDecoder::begin_frame(packet::stream_timestamp_t frame_position,
                             const void* frame_data,
                             size_t frame_size) {
    roc_panic_if_not(frame_data);

    if (frame_data_) {
        roc_panic("pcm decoder: unpaired begin/end");
    }

    frame_data_ = frame_data;
    frame_byte_size_ = frame_size;

    stream_pos_ = frame_position;
    stream_avail_ =
        packet::stream_timestamp_t(pcm_mapper_.input_sample_count(frame_size) / n_chans_);
}

size_t PcmDecoder::read_samples(sample_t* samples, size_t n_samples) {
    if (!frame_data_) {
        roc_panic("pcm decoder: read should be called only between begin/end");
    }

    if (n_samples > (size_t)stream_avail_) {
        n_samples = (size_t)stream_avail_;
    }

    size_t samples_bit_off = 0;

    const size_t n_mapped_samples =
        pcm_mapper_.map(frame_data_, frame_byte_size_, frame_bit_off_, samples,
                        n_samples * n_chans_ * sizeof(sample_t), samples_bit_off,
                        n_samples * n_chans_)
        / n_chans_;

    roc_panic_if_not(samples_bit_off % 8 == 0);
    roc_panic_if_not(n_mapped_samples <= n_samples);

    stream_pos_ += (packet::stream_timestamp_t)n_mapped_samples;
    stream_avail_ -= (packet::stream_timestamp_t)n_mapped_samples;

    return n_mapped_samples;
}

size_t PcmDecoder::drop_samples(size_t n_samples) {
    if (!frame_data_) {
        roc_panic("pcm decoder: shift should be called only between begin/end");
    }

    if (n_samples > (size_t)stream_avail_) {
        n_samples = (size_t)stream_avail_;
    }

    frame_bit_off_ += pcm_mapper_.input_bit_count(n_samples * n_chans_);

    stream_pos_ += (packet::stream_timestamp_t)n_samples;
    stream_avail_ -= (packet::stream_timestamp_t)n_samples;

    return n_samples;
}

void PcmDecoder::end_frame() {
    if (!frame_data_) {
        roc_panic("pcm decoder: unpaired begin/end");
    }

    stream_avail_ = 0;

    frame_data_ = NULL;
    frame_byte_size_ = 0;
    frame_bit_off_ = 0;
}

} // namespace audio
} // namespace roc
