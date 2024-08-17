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

    initialized_ = true;
}

VorbisDecoder::~VorbisDecoder() {
    if (initialized_) {
        vorbis_block_clear(&vorbis_block_);
        vorbis_dsp_clear(&vorbis_dsp_);
        vorbis_info_clear(&vorbis_info_);
        ogg_sync_clear(&ogg_sync_);
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

void VorbisDecoder::begin_frame(packet::stream_timestamp_t frame_position,
                                const void* frame_data,
                                size_t frame_size) {
    roc_panic_if_not(initialized_);

    current_position_ = frame_position;
    available_samples_ = 0;
    current_sample_pos_ = 0;
    total_samples_in_frame_ = 0;

    // Ogg sync to handle the input data
    char* buffer = ogg_sync_buffer(&ogg_sync_, static_cast<long>(frame_size));
    memcpy(buffer, frame_data, frame_size);
    ogg_sync_wrote(&ogg_sync_, static_cast<long>(frame_size));

    // Read headers if not already done
    if (!headers_read_) {
        ogg_page ogg_page;
        while (ogg_sync_pageout(&ogg_sync_, &ogg_page) == 1) {
            ogg_stream_pagein(&ogg_stream_, &ogg_page);

            ogg_packet header_packet;
            while (ogg_stream_packetout(&ogg_stream_, &header_packet) == 1) {
                if (vorbis_synthesis_idheader(&header_packet)) {
                    if (vorbis_synthesis_headerin(&vorbis_info_, &vorbis_comment_,
                                                  &header_packet)
                        < 0) {
                        roc_panic("vorbis decoder: invalid header packet");
                    }
                }

                if (ogg_stream_packetout(&ogg_stream_, &header_packet) != 1) {
                    break;
                }
            }

            // Check if we have read all three headers
            if (vorbis_synthesis_init(&vorbis_dsp_, &vorbis_info_) == 0) {
                vorbis_block_init(&vorbis_dsp_, &vorbis_block_);
                headers_read_ = true;
                break;
            }
        }
    }

    // Extract packets from the buffer
    ogg_page ogg_page;
    while (ogg_sync_pageout(&ogg_sync_, &ogg_page) == 1) {
        ogg_stream_pagein(&ogg_stream_, &ogg_page);
        while (ogg_stream_packetout(&ogg_stream_, &current_packet_) == 1) {
            process_packet_();
        }
    }
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
