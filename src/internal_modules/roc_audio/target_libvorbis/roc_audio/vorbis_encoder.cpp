/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/vorbis_encoder.h"
#include "roc_core/panic.h"
#include <iostream>

namespace roc {
namespace audio {

VorbisEncoder::VorbisEncoder(const SampleSpec& sample_spec, core::IArena& arena)
    : IFrameEncoder(arena)
    , initialized_(false)
    , frame_data_(NULL)
    , frame_size_(0) {
    vorbis_info_init(&vorbis_info_);

    const long num_channels = static_cast<long>(sample_spec.num_channels());
    const long sample_rate = static_cast<long>(sample_spec.sample_rate());
    const float quality = 0.5f;

    if (vorbis_encode_init_vbr(&vorbis_info_, num_channels, sample_rate, quality) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis encoder");
    }

    vorbis_comment_init(&vorbis_comment_);

    if (vorbis_analysis_init(&vorbis_dsp_, &vorbis_info_) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis dsp");
    }

    if (vorbis_block_init(&vorbis_dsp_, &vorbis_block_) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis block");
    }

    ogg_stream_init(&ogg_stream_, 0);
    initialized_ = true;
}

VorbisEncoder::~VorbisEncoder() {
    if (initialized_) {
        ogg_stream_clear(&ogg_stream_);
        vorbis_block_clear(&vorbis_block_);
        vorbis_dsp_clear(&vorbis_dsp_);
        vorbis_comment_clear(&vorbis_comment_);
        vorbis_info_clear(&vorbis_info_);
    }
}

status::StatusCode VorbisEncoder::init_status() const {
    return initialized_ ? status::StatusOK : status::StatusAbort;
}

size_t VorbisEncoder::encoded_byte_count(size_t n_samples) const {
    roc_panic_if_not(initialized_);

    const float nominal_bitrate = vorbis_info_.bitrate_nominal;
    const long num_channels = vorbis_info_.channels;
    const long sample_rate = vorbis_info_.rate;

    // Estimated encoded byte count
    return static_cast<size_t>((nominal_bitrate * n_samples * num_channels)
                               / (sample_rate * 8.0f));
}

void VorbisEncoder::begin_frame(void* frame_data, size_t frame_size) {
    roc_panic_if_not(frame_data);

    if (frame_data_) {
        roc_panic("vorbis encoder: unpaired begin/end");
    }

    frame_data_ = frame_data;
    frame_size_ = frame_size;
}

size_t VorbisEncoder::write_samples(const sample_t* samples, size_t n_samples) {
    roc_panic_if_not(initialized_);

    if (!samples || n_samples == 0) {
        return 0;
    }

    buffer_samples_(samples, n_samples);
    process_analysis_and_encoding_();

    // Indicate that no more samples are to be written
    vorbis_analysis_wrote(&vorbis_dsp_, 0);
    process_analysis_and_encoding_();

    // Vorbis encodes with blocks of fixed amount of samples, if the number of samples to
    // be encoded are less than the block amount of samples, it fill the rest of the block
    // with 0 So the return value is the amount of samples put inside the buffer
    return n_samples;
}

void VorbisEncoder::end_frame() {
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

void VorbisEncoder::process_analysis_and_encoding_() {
    while (vorbis_analysis_blockout(&vorbis_dsp_, &vorbis_block_) == 1) {
        vorbis_analysis(&vorbis_block_, NULL);

        vorbis_bitrate_addblock(&vorbis_block_);

        extract_and_write_packets_();
    }
}

void VorbisEncoder::extract_and_write_packets_() {
    ogg_packet packet;

    while (vorbis_bitrate_flushpacket(&vorbis_dsp_, &packet)) {
        ogg_stream_packetin(&ogg_stream_, &packet);
        write_ogg_pages_();
    }
}

void VorbisEncoder::write_ogg_pages_() {
    long bytes_written = 0;
    ogg_page page;

    while (ogg_stream_pageout(&ogg_stream_, &page)) {
        write_to_frame_(page.header, page.header_len, bytes_written);
        bytes_written += page.header_len;

        write_to_frame_(page.body, page.body_len, bytes_written);
        bytes_written += page.body_len;
    }
}

void VorbisEncoder::write_to_frame_(const void* data, long size, long offset) {
    const size_t casted_size = static_cast<size_t>(size);
    memcpy(static_cast<uint8_t*>(frame_data_) + offset, data, casted_size);
}

} // namespace audio
} // namespace roc
