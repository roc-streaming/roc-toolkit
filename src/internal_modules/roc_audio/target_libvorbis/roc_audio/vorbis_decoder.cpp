/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/vorbis_decoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

VorbisDecoder::VorbisDecoder(const SampleSpec& sample_spec, core::IArena& arena)
    : IFrameDecoder(arena)
    , initialized_(false)
    , current_position_(0)
    , available_samples_(0)
    , pcm_samples_(NULL)
    , current_sample_pos_(0)
    , total_samples_in_frame_(0)
    , headers_read_(false) {
    vorbis_info_init(&vorbis_info_);
    vorbis_comment_init(&vorbis_comment_);
    ogg_sync_init(&ogg_sync_);
    ogg_stream_init(&ogg_stream_, 0);
    initialized_ = true;
}

VorbisDecoder::~VorbisDecoder() {
    if (initialized_) {
        vorbis_block_clear(&vorbis_block_);
        vorbis_dsp_clear(&vorbis_dsp_);
        vorbis_info_clear(&vorbis_info_);
        vorbis_comment_clear(&vorbis_comment_);
        ogg_sync_clear(&ogg_sync_);
        ogg_stream_clear(&ogg_stream_);
    }
}

status::StatusCode VorbisDecoder::init_status() const {
    return initialized_ ? status::StatusOK : status::StatusAbort;
}

packet::stream_timestamp_t VorbisDecoder::position() const {
    return current_position_;
}

packet::stream_timestamp_t VorbisDecoder::available() const {
    return available_samples_;
}

size_t VorbisDecoder::decoded_sample_count(const void* frame_data,
                                           size_t frame_size) const {
    const size_t nominal_bitrate = static_cast<size_t>(vorbis_info_.bitrate_nominal);
    const size_t num_channels = static_cast<size_t>(vorbis_info_.channels);

    return frame_size * 8 / (nominal_bitrate / num_channels);
}

bool VorbisDecoder::initialize_headers(const uint8_t* headers, size_t headers_size) {
    roc_panic_if_not(headers);

    // Reset ogg_sync state to ensure clean reading
    ogg_sync_reset(&ogg_sync_);

    // Add the combined headers to the ogg_sync state
    add_data_to_ogg_sync_(headers, headers_size);

    // Process the headers to initialize decoder state
    if (!read_headers_()) {
        return false;
    }

    headers_read_ = true;
    return true;
}

void VorbisDecoder::begin_frame(packet::stream_timestamp_t frame_position,
                                const void* frame_data,
                                size_t frame_size) {
    roc_panic_if_not(initialized_);

    reset_frame_state_(frame_position);

    add_data_to_ogg_sync_(frame_data, frame_size);

    process_frame_packets_();
}

size_t VorbisDecoder::read_samples(sample_t* samples, size_t n_samples) {
    roc_panic("TODO");
    return 0;
}

size_t VorbisDecoder::drop_samples(size_t n_samples) {
    roc_panic("TODO");
    return 0;
}

void VorbisDecoder::end_frame() {
    roc_panic("TODO");
}

void VorbisDecoder::reset_frame_state_(packet::stream_timestamp_t frame_position) {
    current_position_ = frame_position;
    available_samples_ = 0;
    current_sample_pos_ = 0;
    total_samples_in_frame_ = 0;
}

void VorbisDecoder::add_data_to_ogg_sync_(const void* frame_data, size_t frame_size) {
    char* buffer = ogg_sync_buffer(&ogg_sync_, static_cast<long>(frame_size));
    memcpy(buffer, frame_data, frame_size);
    ogg_sync_wrote(&ogg_sync_, static_cast<long>(frame_size));
}

bool VorbisDecoder::read_headers_() {
    ogg_page ogg_page;
    int header_count = 0;

    // Loop to extract pages from the sync state
    while (ogg_sync_pageout(&ogg_sync_, &ogg_page) == 1) {
        if (ogg_stream_pagein(&ogg_stream_, &ogg_page) < 0) {
            return false;
        }

        ogg_packet header_packet;

        // Loop to extract packets from the stream state
        while (ogg_stream_packetout(&ogg_stream_, &header_packet) == 1) {
            // Pass the header to vorbis_synthesis_headerin regardless of type
            if (vorbis_synthesis_headerin(&vorbis_info_, &vorbis_comment_, &header_packet)
                < 0) {
                return false;
            }

            header_count++;

            // After processing three headers, initialize DSP and block
            if (header_count == 3) {
                if (vorbis_synthesis_init(&vorbis_dsp_, &vorbis_info_) == 0) {
                    if (vorbis_block_init(&vorbis_dsp_, &vorbis_block_) == 0) {
                        headers_read_ = true;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

void VorbisDecoder::process_frame_packets_() {
    ogg_page ogg_page;
    while (ogg_sync_pageout(&ogg_sync_, &ogg_page) == 1) {
        ogg_stream_pagein(&ogg_stream_, &ogg_page);
        while (ogg_stream_packetout(&ogg_stream_, &current_packet_) == 1) {
            process_packet_();
        }
    }
}

void VorbisDecoder::process_packet_() {
    if (vorbis_synthesis(&vorbis_block_, &current_packet_) != 0) {
        return;
    }

    vorbis_synthesis_blockin(&vorbis_dsp_, &vorbis_block_);

    while (true) {
        total_samples_in_frame_ = vorbis_synthesis_pcmout(&vorbis_dsp_, &pcm_samples_);
        if (total_samples_in_frame_ <= 0) {
            break;
        }

        available_samples_ += static_cast<size_t>(total_samples_in_frame_);
        vorbis_synthesis_read(&vorbis_dsp_, total_samples_in_frame_);
    }
}

} // namespace audio
} // namespace roc
