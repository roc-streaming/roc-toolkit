/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_subformat_rw.h
//! @brief PCM sub-format read/write helpers.

#ifndef ROC_AUDIO_PCM_SUBFORMAT_RW_H_
#define ROC_AUDIO_PCM_SUBFORMAT_RW_H_

#include "roc_audio/pcm_subformat.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! Write octet at given byte-aligned bit offset.
inline void pcm_aligned_write(uint8_t* buffer, size_t& bit_offset, uint8_t arg) {
    buffer[bit_offset >> 3] = arg;
    bit_offset += 8;
}

//! Read octet at given byte-aligned bit offset.
inline uint8_t pcm_aligned_read(const uint8_t* buffer, size_t& bit_offset) {
    const uint8_t ret = buffer[bit_offset >> 3];
    bit_offset += 8;
    return ret;
}

//! Write value (at most 8 bits) at given unaligned bit offset.
inline void
pcm_unaligned_write(uint8_t* buffer, size_t& bit_offset, size_t bit_length, uint8_t arg) {
    const size_t byte_index = (bit_offset >> 3);
    const size_t bit_index = (bit_offset & 0x7u);

    if (bit_index == 0) {
        buffer[byte_index] = 0;
    }

    buffer[byte_index] |= uint8_t(uint8_t(arg << (8 - bit_length)) >> bit_index);

    if (bit_index + bit_length > 8) {
        buffer[byte_index + 1] = uint8_t(arg << bit_index);
    }

    bit_offset += bit_length;
}

//! Read value (at most 8 bits) at given unaligned bit offset.
inline uint8_t
pcm_unaligned_read(const uint8_t* buffer, size_t& bit_offset, size_t bit_length) {
    const size_t byte_index = (bit_offset >> 3);
    const size_t bit_index = (bit_offset & 0x7u);

    uint8_t ret = uint8_t(uint8_t(buffer[byte_index] << bit_index) >> (8 - bit_length));

    if (bit_index + bit_length > 8) {
        ret |= uint8_t(buffer[byte_index + 1] >> ((8 - bit_index) + (8 - bit_length)));
    }

    bit_offset += bit_length;
    return ret;
}

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_SUBFORMAT_RW_H_
