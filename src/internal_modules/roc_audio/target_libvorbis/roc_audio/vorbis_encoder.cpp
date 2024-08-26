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
    , current_position_(0)
    , headers_frame_(NULL)
    , headers_frame_size_(0) {
    const long num_channels = static_cast<long>(sample_spec.num_channels());
    const long sample_rate = static_cast<long>(sample_spec.sample_rate());

    initialize_structures_(num_channels, sample_rate);

    create_headers_frame_();

    initialized_ = true;
}

VorbisEncoder::~VorbisEncoder() {
    if (initialized_) {
        vorbis_block_clear(&vorbis_block_);
        vorbis_dsp_clear(&vorbis_dsp_);
        vorbis_info_clear(&vorbis_info_);
        vorbis_comment_clear(&vorbis_comment_);
        ogg_stream_clear(&ogg_stream_);
        if (headers_frame_) {
            free(headers_frame_);
        }
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

const uint8_t* VorbisEncoder::get_headers_frame() const {
    return headers_frame_;
}

size_t VorbisEncoder::get_headers_frame_size() const {
    return headers_frame_size_;
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
    roc_panic_if_not(initialized_);

    // Indicate that no more samples are to be written
    vorbis_analysis_wrote(&vorbis_dsp_, 0);

    // Encode the remaining data
    process_encoding_();

    frame_data_ = NULL;
    frame_size_ = 0;
}

void VorbisEncoder::initialize_structures_(long num_channels, long sample_rate) {
    vorbis_info_init(&vorbis_info_);
    vorbis_comment_init(&vorbis_comment_);

    const float quality = 0.5f;

    // Initialize vorbis_info structure
    if (vorbis_encode_init_vbr(&vorbis_info_, num_channels, sample_rate, quality) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis encoder");
    }

    // Initialize vorbis_dsp_state for encoding
    if (vorbis_analysis_init(&vorbis_dsp_, &vorbis_info_) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis dsp");
    }

    // Initialize ogg_stream_state for the stream
    if (ogg_stream_init(&ogg_stream_, 0) != 0) {
        roc_panic("vorbis encoder: failed to initialize ogg stream");
    }

    // Initialize vorbis_block for encoding
    if (vorbis_block_init(&vorbis_dsp_, &vorbis_block_) != 0) {
        roc_panic("vorbis encoder: failed to initialize vorbis block");
    }
}

void VorbisEncoder::create_headers_frame_() {
    ogg_packet header_packet;
    ogg_packet header_comment;
    ogg_packet header_codebook;

    if (vorbis_analysis_headerout(&vorbis_dsp_, &vorbis_comment_, &header_packet,
                                  &header_comment, &header_codebook)
        != 0) {
        roc_panic("vorbis encoder: failed to create vorbis headers");
    }

    headers_frame_size_ =
        calculate_total_headers_size_(header_packet, header_comment, header_codebook);

    headers_frame_ = static_cast<uint8_t*>(malloc(headers_frame_size_));
    if (!headers_frame_) {
        roc_panic("vorbis encoder: failed to allocate memory for headers");
    }

    copy_headers_to_memory_(header_packet, header_comment, header_codebook);
}

size_t VorbisEncoder::calculate_total_headers_size_(ogg_packet& header_packet,
                                                    ogg_packet& header_comment,
                                                    ogg_packet& header_codebook) {
    ogg_page ogg_page;
    long total_size = 0;

    insert_headers_into_stream_(header_packet, header_comment, header_codebook);

    while (ogg_stream_flush(&ogg_stream_, &ogg_page)) {
        total_size += ogg_page.header_len + ogg_page.body_len;
    }

    return static_cast<size_t>(total_size);
}

void VorbisEncoder::copy_headers_to_memory_(ogg_packet& header_packet,
                                            ogg_packet& header_comment,
                                            ogg_packet& header_codebook) {
    ogg_page ogg_page;
    size_t offset = 0;

    insert_headers_into_stream_(header_packet, header_comment, header_codebook);

    while (ogg_stream_flush(&ogg_stream_, &ogg_page)) {
        const size_t header_len = static_cast<size_t>(ogg_page.header_len);
        const size_t body_len = static_cast<size_t>(ogg_page.body_len);

        memcpy(headers_frame_ + offset, ogg_page.header, header_len);
        offset += header_len;
        memcpy(headers_frame_ + offset, ogg_page.body, body_len);
        offset += body_len;
    }
}

void VorbisEncoder::insert_headers_into_stream_(ogg_packet& header_packet,
                                                ogg_packet& header_comment,
                                                ogg_packet& header_codebook) {
    ogg_stream_reset(&ogg_stream_);
    ogg_stream_packetin(&ogg_stream_, &header_packet);
    ogg_stream_packetin(&ogg_stream_, &header_comment);
    ogg_stream_packetin(&ogg_stream_, &header_codebook);
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
