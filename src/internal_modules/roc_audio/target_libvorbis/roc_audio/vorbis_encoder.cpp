/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/vorbis_encoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

VorbisEncoder::VorbisEncoder(const SampleSpec& sample_spec, core::IArena& arena)
    : IFrameEncoder(arena)
    , initialized_(false)
    , frame_data_(NULL)
    , frame_size_(0)
    , current_position_(0) {
    vorbis_info_init(&vorbis_info_);

    const long num_channels = static_cast<long>(sample_spec.num_channels());
    const long sample_rate = static_cast<long>(sample_spec.sample_rate());
    const float quality = 0.5f;

    if (vorbis_encode_init_vbr(&vorbis_info_, num_channels, sample_rate, quality) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis encoder");
    }

    if (vorbis_analysis_init(&vorbis_dsp_, &vorbis_info_) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis dsp");
    }

    if (vorbis_block_init(&vorbis_dsp_, &vorbis_block_) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis block");
    }

    initialized_ = true;
}

VorbisEncoder::~VorbisEncoder() {
    if (initialized_) {
        vorbis_block_clear(&vorbis_block_);
        vorbis_dsp_clear(&vorbis_dsp_);
        vorbis_info_clear(&vorbis_info_);
    }
}

status::StatusCode VorbisEncoder::init_status() const {
    return initialized_ ? status::StatusOK : status::StatusAbort;
}

size_t VorbisEncoder::encoded_byte_count(size_t n_samples) const {
    roc_panic_if_not(initialized_);

    const size_t nominal_bitrate = static_cast<size_t>(vorbis_info_.bitrate_nominal);
    const size_t num_channels = static_cast<size_t>(vorbis_info_.channels);
    const size_t sample_rate = static_cast<size_t>(vorbis_info_.rate);

    const size_t total_num_bits = nominal_bitrate * n_samples * num_channels;

    // Estimated encoded byte count
    return total_num_bits / (sample_rate * 8);
}

void VorbisEncoder::begin_frame(void* frame_data, size_t frame_size) {
    roc_panic_if_not(frame_data);

    if (frame_data_) {
        roc_panic("vorbis encoder: unpaired begin/end");
    }

    frame_data_ = static_cast<uint8_t*>(frame_data);
    frame_size_ = frame_size;
    current_position_ = 0;
}

size_t VorbisEncoder::write_samples(const sample_t* samples, size_t n_samples) {
    roc_panic_if_not(initialized_);

    if (!samples || n_samples == 0) {
        return 0;
    }

    buffer_samples_(samples, n_samples);

    process_encoding_();

    return n_samples;
}

void VorbisEncoder::end_frame() {
    // Indicate that no more samples are to be written
    vorbis_analysis_wrote(&vorbis_dsp_, 0);

    // Encode the remaining data
    process_encoding_();

    frame_data_ = NULL;
    frame_size_ = 0;
}

void VorbisEncoder::buffer_samples_(const sample_t* samples, size_t n_samples) {
    const int int_n_samples = static_cast<int>(n_samples);

    float** buffer = vorbis_analysis_buffer(&vorbis_dsp_, int_n_samples);

    for (int i = 0; i < int_n_samples; ++i) {
        for (int ch = 0; ch < vorbis_info_.channels; ++ch) {
            buffer[ch][i] = samples[i * vorbis_info_.channels + ch];
        }
    }

    vorbis_analysis_wrote(&vorbis_dsp_, int_n_samples);
}

void VorbisEncoder::process_encoding_() {
    ogg_packet packet;
    while (vorbis_analysis_blockout(&vorbis_dsp_, &vorbis_block_) == 1) {
        vorbis_analysis(&vorbis_block_, 0);
        vorbis_bitrate_addblock(&vorbis_block_);

        while (vorbis_bitrate_flushpacket(&vorbis_dsp_, &packet)) {
            const size_t packet_bytes = static_cast<size_t>(packet.bytes);

            if (current_position_ + packet_bytes > frame_size_) {
                return;
            }

            memcpy(frame_data_ + current_position_, packet.packet, packet_bytes);
            current_position_ += packet_bytes;
        }
    }
}

} // namespace audio
} // namespace roc
