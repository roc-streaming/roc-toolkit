/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_encoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

IFrameEncoder* PcmEncoder::construct(const SampleSpec& sample_spec, core::IArena& arena) {
    return new (arena) PcmEncoder(sample_spec, arena);
}

PcmEncoder::PcmEncoder(const SampleSpec& sample_spec, core::IArena& arena)
    : IFrameEncoder(arena)
    , pcm_mapper_(PcmSubformat_Raw, sample_spec.pcm_subformat())
    , n_chans_(sample_spec.num_channels())
    , frame_data_(NULL)
    , frame_byte_size_(0)
    , frame_bit_off_(0) {
}

status::StatusCode PcmEncoder::init_status() const {
    return status::StatusOK;
}

size_t PcmEncoder::encoded_byte_count(size_t num_samples) const {
    return pcm_mapper_.output_byte_count(num_samples * n_chans_);
}

void PcmEncoder::begin_frame(void* frame_data, size_t frame_size) {
    roc_panic_if_not(frame_data);

    if (frame_data_) {
        roc_panic("pcm encoder: unpaired begin/end");
    }

    frame_data_ = frame_data;
    frame_byte_size_ = frame_size;
}

size_t PcmEncoder::write_samples(const sample_t* samples, size_t n_samples) {
    if (!frame_data_) {
        roc_panic("pcm encoder: write should be called only between begin/end");
    }

    size_t samples_bit_off = 0;

    const size_t n_mapped_samples =
        pcm_mapper_.map(samples, n_samples * n_chans_ * sizeof(sample_t), samples_bit_off,
                        frame_data_, frame_byte_size_, frame_bit_off_,
                        n_samples * n_chans_)
        / n_chans_;

    roc_panic_if_not(samples_bit_off % 8 == 0);
    roc_panic_if_not(n_mapped_samples <= n_samples);

    return n_mapped_samples;
}

void PcmEncoder::end_frame() {
    if (!frame_data_) {
        roc_panic("pcm encoder: unpaired begin/end");
    }

    frame_data_ = NULL;
    frame_byte_size_ = 0;
    frame_bit_off_ = 0;
}

} // namespace audio
} // namespace roc
