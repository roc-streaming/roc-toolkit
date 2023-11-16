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

#include "roc_core/attributes.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! PCM sample binary code.
enum PcmCode {
    PcmCode_SInt8,    //!< 8-bit signed integer.
    PcmCode_UInt8,    //!< 8-bit unsigned integer.
    PcmCode_SInt16,   //!< 16-bit signed integer.
    PcmCode_UInt16,   //!< 16-bit unsigned integer.
    PcmCode_SInt18,   //!< 18-bit signed integer (2.25 bytes).
    PcmCode_UInt18,   //!< 18-bit unsigned integer (2.25 bytes).
    PcmCode_SInt18_3, //!< 18-bit signed integer, in low bits of 3-byte container.
    PcmCode_UInt18_3, //!< 18-bit unsigned integer, in low bits of 3-byte container.
    PcmCode_SInt18_4, //!< 18-bit signed integer, in low bits of 4-byte container.
    PcmCode_UInt18_4, //!< 18-bit unsigned integer, in low bits of 4-byte container.
    PcmCode_SInt20,   //!< 20-bit signed integer (2.5 bytes).
    PcmCode_UInt20,   //!< 20-bit unsigned integer (2.5 bytes).
    PcmCode_SInt20_3, //!< 20-bit signed integer, in low bits of 3-byte container.
    PcmCode_UInt20_3, //!< 20-bit unsigned integer, in low bits of 3-byte container.
    PcmCode_SInt20_4, //!< 20-bit signed integer, in low bits of 4-byte container.
    PcmCode_UInt20_4, //!< 20-bit unsigned integer, in low bits of 4-byte container.
    PcmCode_SInt24,   //!< 24-bit signed integer (3 bytes).
    PcmCode_UInt24,   //!< 24-bit unsigned integer (3 bytes).
    PcmCode_SInt24_4, //!< 24-bit signed integer, in low bits of 4-byte container.
    PcmCode_UInt24_4, //!< 24-bit unsigned integer, in low bits of 4-byte container.
    PcmCode_SInt32,   //!< 32-bit signed integer.
    PcmCode_UInt32,   //!< 32-bit unsigned integer.
    PcmCode_SInt64,   //!< 64-bit signed integer.
    PcmCode_UInt64,   //!< 64-bit unsigned integer.
    PcmCode_Float32,  //!< 32-bit IEEE-754 float in range [-1.0; +1.0].
    PcmCode_Float64,  //!< 64-bit IEEE-754 float in range [-1.0; +1.0].

    PcmCode_Max //!< Maximum value.
};

//! PCM sample endianess.
enum PcmEndian {
    PcmEndian_Native, //!< Endian native to current CPU.
    PcmEndian_Big,    //!< Big endian.
    PcmEndian_Little, //!< Little endian.

    PcmEndian_Max //!< Maximum value.
};

//! PCM format description.
struct PcmFormat {
    //! Sample binary code.
    PcmCode code;

    //! Sample endianess.
    PcmEndian endian;

    //! Initialize.
    PcmFormat()
        : code()
        , endian() {
    }

    //! Initialize.
    PcmFormat(PcmCode enc, PcmEndian end)
        : code(enc)
        , endian(end) {
    }

    //! Check two formats for equality.
    bool operator==(const PcmFormat& other) const {
        return code == other.code && endian == other.endian;
    }

    //! Check two formats for equality.
    bool operator!=(const PcmFormat& other) const {
        return !(*this == other);
    }
};

//! PCM format meta-information.
struct PcmTraits {
    //! Number of significant bits per sample.
    size_t bit_depth;

    //! Number of total bits per sample in packed form.
    size_t bit_width;

    //! True for integers, false for floating point.
    bool is_integer;

    //! True for signed integers and floating point.
    bool is_signed;

    PcmTraits()
        : bit_depth(0)
        , bit_width(0)
        , is_integer(false)
        , is_signed(false) {
    }
};

//! Get string name of PCM format.
const char* pcm_format_to_str(const PcmFormat& fmt);

//! Parse PCM format from string name.
ROC_ATTR_NODISCARD bool pcm_format_parse(const char* str, PcmFormat& fmt);

//! Get traits for PCM format.
PcmTraits pcm_format_traits(const PcmFormat& fmt);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_FORMAT_H_
