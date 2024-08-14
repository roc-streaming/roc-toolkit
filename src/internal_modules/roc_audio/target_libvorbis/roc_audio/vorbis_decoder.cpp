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
    : IFrameDecoder(arena) {
    // TODO
}

VorbisDecoder::~VorbisDecoder() {
    // TODO
}

status::StatusCode VorbisDecoder::init_status() const {
    return status::StatusOK;
}

packet::stream_timestamp_t VorbisDecoder::position() const {
    roc_panic("TODO");
    return 0;
}

packet::stream_timestamp_t VorbisDecoder::available() const {
    roc_panic("TODO");
    return 0;
}

size_t VorbisDecoder::decoded_sample_count(const void* frame_data,
                                           size_t frame_size) const {
    roc_panic("TODO");
    return 0;
}

void VorbisDecoder::begin_frame(packet::stream_timestamp_t frame_position,
                                const void* frame_data,
                                size_t frame_size) {
    roc_panic_if_not(frame_data);
    roc_panic("TODO");
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

} // namespace audio
} // namespace roc
