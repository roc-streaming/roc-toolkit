/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_format.h
//! @brief PCM format.

#ifndef ROC_AUDIO_PCM_FORMAT_H_
#define ROC_AUDIO_PCM_FORMAT_H_

#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! PCM sample encoding.
enum PcmEncoding {
    PcmEncoding_SInt8,     //!< 8-bit signed integer.
    PcmEncoding_UInt8,     //!< 8-bit unsigned integer.
    PcmEncoding_SInt16,    //!< 16-bit signed integer.
    PcmEncoding_UInt16,    //!< 16-bit unsigned integer.
    PcmEncoding_SInt18,    //!< 18-bit signed integer (2.25 bytes).
    PcmEncoding_UInt18,    //!< 18-bit unsigned integer (2.25 bytes).
    PcmEncoding_SInt18_3B, //!< 18-bit signed integer, in low bits of 3-byte container.
    PcmEncoding_UInt18_3B, //!< 18-bit unsigned integer, in low bits of 3-byte container.
    PcmEncoding_SInt18_4B, //!< 18-bit signed integer, in low bits of 4-byte container.
    PcmEncoding_UInt18_4B, //!< 18-bit unsigned integer, in low bits of 4-byte container.
    PcmEncoding_SInt20,    //!< 20-bit signed integer (2.5 bytes).
    PcmEncoding_UInt20,    //!< 20-bit unsigned integer (2.5 bytes).
    PcmEncoding_SInt20_3B, //!< 20-bit signed integer, in low bits of 3-byte container.
    PcmEncoding_UInt20_3B, //!< 20-bit unsigned integer, in low bits of 3-byte container.
    PcmEncoding_SInt20_4B, //!< 20-bit signed integer, in low bits of 4-byte container.
    PcmEncoding_UInt20_4B, //!< 20-bit unsigned integer, in low bits of 4-byte container.
    PcmEncoding_SInt24,    //!< 24-bit signed integer (3 bytes).
    PcmEncoding_UInt24,    //!< 24-bit unsigned integer (3 bytes).
    PcmEncoding_SInt24_4B, //!< 24-bit signed integer, in low bits of 4-byte container.
    PcmEncoding_UInt24_4B, //!< 24-bit unsigned integer, in low bits of 4-byte container.
    PcmEncoding_SInt32,    //!< 32-bit signed integer.
    PcmEncoding_UInt32,    //!< 32-bit unsigned integer.
    PcmEncoding_SInt64,    //!< 64-bit signed integer.
    PcmEncoding_UInt64,    //!< 64-bit unsigned integer.
    PcmEncoding_Float32,   //!< 32-bit IEEE-754 float in range [-1.0; +1.0].
    PcmEncoding_Float64    //!< 64-bit IEEE-754 float in range [-1.0; +1.0].
};

//! PCM sample endianess.
enum PcmEndian {
    PcmEndian_Native, //!< Endian native to current CPU.
    PcmEndian_Big,    //!< Big endian.
    PcmEndian_Little  //!< Little endian.
};

//! PCM format description.
struct PcmFormat {
    //! Sample encoding.
    PcmEncoding encoding;

    //! Sample endian.
    PcmEndian endian;

    //! Initialize.
    PcmFormat()
        : encoding()
        , endian() {
    }
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_FORMAT_H_
