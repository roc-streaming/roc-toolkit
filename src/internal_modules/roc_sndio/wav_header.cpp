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

WavHeader::WavHeader(uint16_t num_channels,
                     uint32_t sample_rate,
                     uint16_t bits_per_sample)
    : data_(
        core::EndianOps::swap_native_be<uint32_t>(0x52494646), // {'R','I','F','F'}
        core::EndianOps::swap_native_be<uint32_t>(0x57415645), // {'W','A','V','E'}
        core::EndianOps::swap_native_be<uint32_t>(0x666d7420), // {'f','m','t',' '}
        core::EndianOps::swap_native_le<uint16_t>(0x10),       // 16
        core::EndianOps::swap_native_le<uint16_t>(0x3),        // IEEE Float
        core::EndianOps::swap_native_le(num_channels),
        core::EndianOps::swap_native_le<uint32_t>(sample_rate),
        core::EndianOps::swap_native_le<uint32_t>(sample_rate * num_channels
                                                  * (bits_per_sample / 8u)),
        core::EndianOps::swap_native_le<uint16_t>(num_channels * (bits_per_sample / 8u)),
        core::EndianOps::swap_native_le<uint16_t>(bits_per_sample),
        core::EndianOps::swap_native_be<uint32_t>(0x64617461) /* {'d','a','t','a'} */) {
}

WavHeader::WavHeaderData::WavHeaderData(uint32_t chunk_id,
                                        uint32_t format,
                                        uint32_t subchunk1_id,
                                        uint32_t subchunk1_size,
                                        uint16_t audio_format,
                                        uint16_t num_channels,
                                        uint32_t sample_rate,
                                        uint32_t byte_rate,
                                        uint16_t block_align,
                                        uint16_t bits_per_sample,
                                        uint32_t subchunk2_id)
    : chunk_id_(chunk_id)
    , format_(format)
    , subchunk1_id_(subchunk1_id)
    , subchunk1_size_(subchunk1_size)
    , audio_format_(audio_format)
    , num_channels_(num_channels)
    , sample_rate_(sample_rate)
    , byte_rate_(byte_rate)
    , block_align_(block_align)
    , bits_per_sample_(bits_per_sample)
    , subchunk2_id_(subchunk2_id) {
}

uint16_t WavHeader::num_channels() const {
    return data_.num_channels_;
}

uint32_t WavHeader::sample_rate() const {
    return data_.sample_rate_;
}

uint16_t WavHeader::bits_per_sample() const {
    return data_.bits_per_sample_;
}

void WavHeader::reset_sample_counter(uint32_t num_samples) {
    num_samples_ = num_samples;
}

const WavHeader::WavHeaderData& WavHeader::update_and_get_header(uint32_t num_samples) {
    num_samples_ += num_samples;
    data_.subchunk2_size_ =
        num_samples_ * data_.num_channels_ * (data_.bits_per_sample_ / 8u);
    data_.chunk_size_ = METADATA_SIZE_ + data_.subchunk2_size_;

    return data_;
}

} // namespace sndio
} // namespace roc
