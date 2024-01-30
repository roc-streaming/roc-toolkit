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

#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! PCM format.
//! Defines PCM sample coding and endian.
enum PcmFormat {
    //! Invalid format.
    PcmFormat_Invalid,

    //! 8-bit signed integer, native endian.
    PcmFormat_SInt8,
    //! 8-bit signed integer, big endian.
    PcmFormat_SInt8_Be,
    //! 8-bit signed integer, little endian.
    PcmFormat_SInt8_Le,
    //! 8-bit unsigned integer, native endian.
    PcmFormat_UInt8,
    //! 8-bit unsigned integer, big endian.
    PcmFormat_UInt8_Be,
    //! 8-bit unsigned integer, little endian.
    PcmFormat_UInt8_Le,

    //! 16-bit signed integer, native endian.
    PcmFormat_SInt16,
    //! 16-bit signed integer, big endian.
    PcmFormat_SInt16_Be,
    //! 16-bit signed integer, little endian.
    PcmFormat_SInt16_Le,
    //! 16-bit unsigned integer, native endian.
    PcmFormat_UInt16,
    //! 16-bit unsigned integer, big endian.
    PcmFormat_UInt16_Be,
    //! 16-bit unsigned integer, little endian.
    PcmFormat_UInt16_Le,

    //! 18-bit signed integer (2.25 bytes), native endian.
    PcmFormat_SInt18,
    //! 18-bit signed integer (2.25 bytes), big endian.
    PcmFormat_SInt18_Be,
    //! 18-bit signed integer (2.25 bytes), little endian.
    PcmFormat_SInt18_Le,
    //! 18-bit unsigned integer (2.25 bytes), native endian.
    PcmFormat_UInt18,
    //! 18-bit unsigned integer (2.25 bytes), big endian.
    PcmFormat_UInt18_Be,
    //! 18-bit unsigned integer (2.25 bytes), little endian.
    PcmFormat_UInt18_Le,

    //! 18-bit signed integer, in low bits of 3-byte container, native endian.
    PcmFormat_SInt18_3,
    //! 18-bit signed integer, in low bits of 3-byte container, big endian.
    PcmFormat_SInt18_3_Be,
    //! 18-bit signed integer, in low bits of 3-byte container, little endian.
    PcmFormat_SInt18_3_Le,
    //! 18-bit unsigned integer, in low bits of 3-byte container, native endian.
    PcmFormat_UInt18_3,
    //! 18-bit unsigned integer, in low bits of 3-byte container, big endian.
    PcmFormat_UInt18_3_Be,
    //! 18-bit unsigned integer, in low bits of 3-byte container, little endian.
    PcmFormat_UInt18_3_Le,

    //! 18-bit signed integer, in low bits of 4-byte container, native endian.
    PcmFormat_SInt18_4,
    //! 18-bit signed integer, in low bits of 4-byte container, big endian.
    PcmFormat_SInt18_4_Be,
    //! 18-bit signed integer, in low bits of 4-byte container, little endian.
    PcmFormat_SInt18_4_Le,
    //! 18-bit unsigned integer, in low bits of 4-byte container, native endian.
    PcmFormat_UInt18_4,
    //! 18-bit unsigned integer, in low bits of 4-byte container, big endian.
    PcmFormat_UInt18_4_Be,
    //! 18-bit unsigned integer, in low bits of 4-byte container, little endian.
    PcmFormat_UInt18_4_Le,

    //! 20-bit signed integer (2.5 bytes), native endian.
    PcmFormat_SInt20,
    //! 20-bit signed integer (2.5 bytes), big endian.
    PcmFormat_SInt20_Be,
    //! 20-bit signed integer (2.5 bytes), little endian.
    PcmFormat_SInt20_Le,
    //! 20-bit unsigned integer (2.5 bytes), native endian.
    PcmFormat_UInt20,
    //! 20-bit unsigned integer (2.5 bytes), big endian.
    PcmFormat_UInt20_Be,
    //! 20-bit unsigned integer (2.5 bytes), little endian.
    PcmFormat_UInt20_Le,

    //! 20-bit signed integer, in low bits of 3-byte container, native endian.
    PcmFormat_SInt20_3,
    //! 20-bit signed integer, in low bits of 3-byte container, big endian.
    PcmFormat_SInt20_3_Be,
    //! 20-bit signed integer, in low bits of 3-byte container, little endian.
    PcmFormat_SInt20_3_Le,
    //! 20-bit unsigned integer, in low bits of 3-byte container, native endian.
    PcmFormat_UInt20_3,
    //! 20-bit unsigned integer, in low bits of 3-byte container, big endian.
    PcmFormat_UInt20_3_Be,
    //! 20-bit unsigned integer, in low bits of 3-byte container, little endian.
    PcmFormat_UInt20_3_Le,

    //! 20-bit signed integer, in low bits of 4-byte container, native endian.
    PcmFormat_SInt20_4,
    //! 20-bit signed integer, in low bits of 4-byte container, big endian.
    PcmFormat_SInt20_4_Be,
    //! 20-bit signed integer, in low bits of 4-byte container, little endian.
    PcmFormat_SInt20_4_Le,
    //! 20-bit unsigned integer, in low bits of 4-byte container, native endian.
    PcmFormat_UInt20_4,
    //! 20-bit unsigned integer, in low bits of 4-byte container, big endian.
    PcmFormat_UInt20_4_Be,
    //! 20-bit unsigned integer, in low bits of 4-byte container, little endian.
    PcmFormat_UInt20_4_Le,

    //! 24-bit signed integer (3 bytes), native endian.
    PcmFormat_SInt24,
    //! 24-bit signed integer (3 bytes), big endian.
    PcmFormat_SInt24_Be,
    //! 24-bit signed integer (3 bytes), little endian.
    PcmFormat_SInt24_Le,
    //! 24-bit unsigned integer (3 bytes), native endian.
    PcmFormat_UInt24,
    //! 24-bit unsigned integer (3 bytes), big endian.
    PcmFormat_UInt24_Be,
    //! 24-bit unsigned integer (3 bytes), little endian.
    PcmFormat_UInt24_Le,

    //! 24-bit signed integer, in low bits of 4-byte container, native endian.
    PcmFormat_SInt24_4,
    //! 24-bit signed integer, in low bits of 4-byte container, big endian.
    PcmFormat_SInt24_4_Be,
    //! 24-bit signed integer, in low bits of 4-byte container, little endian.
    PcmFormat_SInt24_4_Le,
    //! 24-bit unsigned integer, in low bits of 4-byte container, native endian.
    PcmFormat_UInt24_4,
    //! 24-bit unsigned integer, in low bits of 4-byte container, big endian.
    PcmFormat_UInt24_4_Be,
    //! 24-bit unsigned integer, in low bits of 4-byte container, little endian.
    PcmFormat_UInt24_4_Le,

    //! 32-bit signed integer, native endian.
    PcmFormat_SInt32,
    //! 32-bit signed integer, big endian.
    PcmFormat_SInt32_Be,
    //! 32-bit signed integer, little endian.
    PcmFormat_SInt32_Le,
    //! 32-bit unsigned integer, native endian.
    PcmFormat_UInt32,
    //! 32-bit unsigned integer, big endian.
    PcmFormat_UInt32_Be,
    //! 32-bit unsigned integer, little endian.
    PcmFormat_UInt32_Le,

    //! 64-bit signed integer, native endian.
    PcmFormat_SInt64,
    //! 64-bit signed integer, big endian.
    PcmFormat_SInt64_Be,
    //! 64-bit signed integer, little endian.
    PcmFormat_SInt64_Le,
    //! 64-bit unsigned integer, native endian.
    PcmFormat_UInt64,
    //! 64-bit unsigned integer, big endian.
    PcmFormat_UInt64_Be,
    //! 64-bit unsigned integer, little endian.
    PcmFormat_UInt64_Le,

    //! 32-bit IEEE-754 float in range [-1.0; +1.0], native endian.
    PcmFormat_Float32,
    //! 32-bit IEEE-754 float in range [-1.0; +1.0], big endian.
    PcmFormat_Float32_Be,
    //! 32-bit IEEE-754 float in range [-1.0; +1.0], little endian.
    PcmFormat_Float32_Le,

    //! 64-bit IEEE-754 float in range [-1.0; +1.0], native endian.
    PcmFormat_Float64,
    //! 64-bit IEEE-754 float in range [-1.0; +1.0], big endian.
    PcmFormat_Float64_Be,
    //! 64-bit IEEE-754 float in range [-1.0; +1.0], little endian.
    PcmFormat_Float64_Le,

    //! Maximum enum value.
    PcmFormat_Max
};

//! PCM format meta-information.
struct PcmTraits {
    //! If traits are valid.
    bool is_valid;

    //! True for integers, false for floating point.
    bool is_integer;

    //! True for signed integers and floating point.
    bool is_signed;

    //! True for little-endian formats.
    bool is_little;

    //! Canonical identifier of the same format.
    PcmFormat canon_id;

    //! Number of significant bits per sample.
    size_t bit_depth;

    //! Number of stored bits per sample in packed form.
    size_t bit_width;

    PcmTraits()
        : is_valid(false)
        , is_integer(false)
        , is_signed(false)
        , is_little(false)
        , canon_id(PcmFormat_Invalid)
        , bit_depth(0)
        , bit_width(0) {
    }
};

//! PCM mapping function.
typedef void (*PcmMapFn)(const uint8_t* in_data,
                         size_t& in_bit_off,
                         uint8_t* out_data,
                         size_t& out_bit_off,
                         size_t n_samples);

//! Get mapping function for given PCM format pair.
PcmMapFn pcm_format_mapfn(PcmFormat in_format, PcmFormat out_format);

//! Get format traits for given PCM format.
PcmTraits pcm_format_traits(PcmFormat format);

//! Get string name of PCM format.
const char* pcm_format_to_str(PcmFormat format);

//! Get PCM format from string name.
PcmFormat pcm_format_from_str(const char* str);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_FORMAT_H_
