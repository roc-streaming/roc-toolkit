/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "wav_header.h"
#include "roc_core/endian_ops.h"

namespace roc {
namespace sndio {

WavHeader::WavHeader(const uint16_t format_tag,
                     const uint16_t bits_per_sample,
                     const uint32_t sample_rate,
                     const uint16_t num_channels)
    : data_(
        // chunk_id
        core::EndianOps::swap_native_be<uint32_t>(0x52494646), // {'R','I','F','F'}
        // chunk_size
        0,
        // form_type
        core::EndianOps::swap_native_be<uint32_t>(0x57415645), // {'W','A','V','E'}
        // subchunk1_id
        core::EndianOps::swap_native_be<uint32_t>(0x666d7420), // {'f','m','t',' '}
        // subchunk1_size
        core::EndianOps::swap_native_le<uint16_t>(16),
        // audio_format
        core::EndianOps::swap_native_le<uint16_t>(format_tag),
        // num_channels
        core::EndianOps::swap_native_le(num_channels),
        // sample_rate
        core::EndianOps::swap_native_le<uint32_t>(sample_rate),
        // byte_rate
        core::EndianOps::swap_native_le<uint32_t>(sample_rate * num_channels
                                                  * (bits_per_sample / 8u)),
        // block_align
        core::EndianOps::swap_native_le<uint16_t>(num_channels * (bits_per_sample / 8u)),
        // bits_per_sample
        core::EndianOps::swap_native_le<uint16_t>(bits_per_sample),
        // subchunk2_id
        core::EndianOps::swap_native_be<uint32_t>(0x64617461), // {'d','a','t','a'}
        // subchunk2_size
        0) {
}

WavHeader::WavHeaderData::WavHeaderData(uint32_t chunk_id,
                                        uint32_t chunk_size,
                                        uint32_t form_type,
                                        uint32_t subchunk1_id,
                                        uint32_t subchunk1_size,
                                        uint16_t audio_format,
                                        uint16_t num_channels,
                                        uint32_t sample_rate,
                                        uint32_t byte_rate,
                                        uint16_t block_align,
                                        uint16_t bits_per_sample,
                                        uint32_t subchunk2_id,
                                        uint32_t subchunk2_size)
    : chunk_id(chunk_id)
    , chunk_size(chunk_size)
    , form_type(form_type)
    , subchunk1_id(subchunk1_id)
    , subchunk1_size(subchunk1_size)
    , audio_format(audio_format)
    , num_channels(num_channels)
    , sample_rate(sample_rate)
    , byte_rate(byte_rate)
    , block_align(block_align)
    , bits_per_sample(bits_per_sample)
    , subchunk2_id(subchunk2_id)
    , subchunk2_size(subchunk2_size) {
}

uint16_t WavHeader::num_channels() const {
    return data_.num_channels;
}

uint32_t WavHeader::sample_rate() const {
    return data_.sample_rate;
}

uint16_t WavHeader::bits_per_sample() const {
    return data_.bits_per_sample;
}

void WavHeader::reset_sample_counter(uint32_t num_samples) {
    num_samples_ = num_samples;
}

const WavHeader::WavHeaderData& WavHeader::update_and_get_header(uint32_t num_samples) {
    num_samples_ += num_samples;
    data_.subchunk2_size =
        num_samples_ * data_.num_channels * (data_.bits_per_sample / 8u);
    data_.chunk_size = METADATA_SIZE_ + data_.subchunk2_size;

    return data_;
}

} // namespace sndio
} // namespace roc
