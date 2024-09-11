/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_subformat.h
//! @brief PCM sub-format.

#ifndef ROC_AUDIO_PCM_SUBFORMAT_H_
#define ROC_AUDIO_PCM_SUBFORMAT_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! PCM sub-format.
//!
//! Defines PCM sample coding and endian.
//! Used when Format is set to Format_Pcm or other PCM-based format like
//! FLAC, ALAC, WAV, etc.
//!
//! "Default endian" means use whatever endian is natural in current context, e.g.:
//!  - for network packets, big endian (a.k.a. network byte order) is default
//!  - for processing, native CPU endian is default
//!  - for WAV files, little endian is default
enum PcmSubformat {
    //! Invalid format.
    PcmSubformat_Invalid,

    //! 8-bit signed integer, default endian.
    PcmSubformat_SInt8,
    //! 8-bit signed integer, big endian.
    PcmSubformat_SInt8_Be,
    //! 8-bit signed integer, little endian.
    PcmSubformat_SInt8_Le,
    //! 8-bit unsigned integer, default endian.
    PcmSubformat_UInt8,
    //! 8-bit unsigned integer, big endian.
    PcmSubformat_UInt8_Be,
    //! 8-bit unsigned integer, little endian.
    PcmSubformat_UInt8_Le,

    //! 16-bit signed integer, default endian.
    PcmSubformat_SInt16,
    //! 16-bit signed integer, big endian.
    PcmSubformat_SInt16_Be,
    //! 16-bit signed integer, little endian.
    PcmSubformat_SInt16_Le,
    //! 16-bit unsigned integer, default endian.
    PcmSubformat_UInt16,
    //! 16-bit unsigned integer, big endian.
    PcmSubformat_UInt16_Be,
    //! 16-bit unsigned integer, little endian.
    PcmSubformat_UInt16_Le,

    //! 18-bit signed integer (2.25 bytes), default endian.
    PcmSubformat_SInt18,
    //! 18-bit signed integer (2.25 bytes), big endian.
    PcmSubformat_SInt18_Be,
    //! 18-bit signed integer (2.25 bytes), little endian.
    PcmSubformat_SInt18_Le,
    //! 18-bit unsigned integer (2.25 bytes), default endian.
    PcmSubformat_UInt18,
    //! 18-bit unsigned integer (2.25 bytes), big endian.
    PcmSubformat_UInt18_Be,
    //! 18-bit unsigned integer (2.25 bytes), little endian.
    PcmSubformat_UInt18_Le,

    //! 18-bit signed integer, in low bits of 3-byte container, default endian.
    PcmSubformat_SInt18_3,
    //! 18-bit signed integer, in low bits of 3-byte container, big endian.
    PcmSubformat_SInt18_3_Be,
    //! 18-bit signed integer, in low bits of 3-byte container, little endian.
    PcmSubformat_SInt18_3_Le,
    //! 18-bit unsigned integer, in low bits of 3-byte container, default endian.
    PcmSubformat_UInt18_3,
    //! 18-bit unsigned integer, in low bits of 3-byte container, big endian.
    PcmSubformat_UInt18_3_Be,
    //! 18-bit unsigned integer, in low bits of 3-byte container, little endian.
    PcmSubformat_UInt18_3_Le,

    //! 18-bit signed integer, in low bits of 4-byte container, default endian.
    PcmSubformat_SInt18_4,
    //! 18-bit signed integer, in low bits of 4-byte container, big endian.
    PcmSubformat_SInt18_4_Be,
    //! 18-bit signed integer, in low bits of 4-byte container, little endian.
    PcmSubformat_SInt18_4_Le,
    //! 18-bit unsigned integer, in low bits of 4-byte container, default endian.
    PcmSubformat_UInt18_4,
    //! 18-bit unsigned integer, in low bits of 4-byte container, big endian.
    PcmSubformat_UInt18_4_Be,
    //! 18-bit unsigned integer, in low bits of 4-byte container, little endian.
    PcmSubformat_UInt18_4_Le,

    //! 20-bit signed integer (2.5 bytes), default endian.
    PcmSubformat_SInt20,
    //! 20-bit signed integer (2.5 bytes), big endian.
    PcmSubformat_SInt20_Be,
    //! 20-bit signed integer (2.5 bytes), little endian.
    PcmSubformat_SInt20_Le,
    //! 20-bit unsigned integer (2.5 bytes), default endian.
    PcmSubformat_UInt20,
    //! 20-bit unsigned integer (2.5 bytes), big endian.
    PcmSubformat_UInt20_Be,
    //! 20-bit unsigned integer (2.5 bytes), little endian.
    PcmSubformat_UInt20_Le,

    //! 20-bit signed integer, in low bits of 3-byte container, default endian.
    PcmSubformat_SInt20_3,
    //! 20-bit signed integer, in low bits of 3-byte container, big endian.
    PcmSubformat_SInt20_3_Be,
    //! 20-bit signed integer, in low bits of 3-byte container, little endian.
    PcmSubformat_SInt20_3_Le,
    //! 20-bit unsigned integer, in low bits of 3-byte container, default endian.
    PcmSubformat_UInt20_3,
    //! 20-bit unsigned integer, in low bits of 3-byte container, big endian.
    PcmSubformat_UInt20_3_Be,
    //! 20-bit unsigned integer, in low bits of 3-byte container, little endian.
    PcmSubformat_UInt20_3_Le,

    //! 20-bit signed integer, in low bits of 4-byte container, default endian.
    PcmSubformat_SInt20_4,
    //! 20-bit signed integer, in low bits of 4-byte container, big endian.
    PcmSubformat_SInt20_4_Be,
    //! 20-bit signed integer, in low bits of 4-byte container, little endian.
    PcmSubformat_SInt20_4_Le,
    //! 20-bit unsigned integer, in low bits of 4-byte container, default endian.
    PcmSubformat_UInt20_4,
    //! 20-bit unsigned integer, in low bits of 4-byte container, big endian.
    PcmSubformat_UInt20_4_Be,
    //! 20-bit unsigned integer, in low bits of 4-byte container, little endian.
    PcmSubformat_UInt20_4_Le,

    //! 24-bit signed integer (3 bytes), default endian.
    PcmSubformat_SInt24,
    //! 24-bit signed integer (3 bytes), big endian.
    PcmSubformat_SInt24_Be,
    //! 24-bit signed integer (3 bytes), little endian.
    PcmSubformat_SInt24_Le,
    //! 24-bit unsigned integer (3 bytes), default endian.
    PcmSubformat_UInt24,
    //! 24-bit unsigned integer (3 bytes), big endian.
    PcmSubformat_UInt24_Be,
    //! 24-bit unsigned integer (3 bytes), little endian.
    PcmSubformat_UInt24_Le,

    //! 24-bit signed integer, in low bits of 4-byte container, default endian.
    PcmSubformat_SInt24_4,
    //! 24-bit signed integer, in low bits of 4-byte container, big endian.
    PcmSubformat_SInt24_4_Be,
    //! 24-bit signed integer, in low bits of 4-byte container, little endian.
    PcmSubformat_SInt24_4_Le,
    //! 24-bit unsigned integer, in low bits of 4-byte container, default endian.
    PcmSubformat_UInt24_4,
    //! 24-bit unsigned integer, in low bits of 4-byte container, big endian.
    PcmSubformat_UInt24_4_Be,
    //! 24-bit unsigned integer, in low bits of 4-byte container, little endian.
    PcmSubformat_UInt24_4_Le,

    //! 32-bit signed integer, default endian.
    PcmSubformat_SInt32,
    //! 32-bit signed integer, big endian.
    PcmSubformat_SInt32_Be,
    //! 32-bit signed integer, little endian.
    PcmSubformat_SInt32_Le,
    //! 32-bit unsigned integer, default endian.
    PcmSubformat_UInt32,
    //! 32-bit unsigned integer, big endian.
    PcmSubformat_UInt32_Be,
    //! 32-bit unsigned integer, little endian.
    PcmSubformat_UInt32_Le,

    //! 64-bit signed integer, default endian.
    PcmSubformat_SInt64,
    //! 64-bit signed integer, big endian.
    PcmSubformat_SInt64_Be,
    //! 64-bit signed integer, little endian.
    PcmSubformat_SInt64_Le,
    //! 64-bit unsigned integer, default endian.
    PcmSubformat_UInt64,
    //! 64-bit unsigned integer, big endian.
    PcmSubformat_UInt64_Be,
    //! 64-bit unsigned integer, little endian.
    PcmSubformat_UInt64_Le,

    //! 32-bit IEEE-754 float in range [-1.0; +1.0], default endian.
    PcmSubformat_Float32,
    //! 32-bit IEEE-754 float in range [-1.0; +1.0], big endian.
    PcmSubformat_Float32_Be,
    //! 32-bit IEEE-754 float in range [-1.0; +1.0], little endian.
    PcmSubformat_Float32_Le,

    //! 64-bit IEEE-754 float in range [-1.0; +1.0], default endian.
    PcmSubformat_Float64,
    //! 64-bit IEEE-754 float in range [-1.0; +1.0], big endian.
    PcmSubformat_Float64_Be,
    //! 64-bit IEEE-754 float in range [-1.0; +1.0], little endian.
    PcmSubformat_Float64_Le,

    //! Maximum enum value.
    PcmSubformat_Max
};

//! PCM format flags.
enum PcmFlags {
    //! Set for signed or unsigned integers.
    Pcm_IsInteger = (1 << 0),
    //! Set for floats and doubles.
    Pcm_IsFloat = (1 << 1),
    //! Set for signed integers, floats, and doubles.
    Pcm_IsSigned = (1 << 2),

    //! Set if format is densely packed (has no padding bits).
    //! E.g. s18 is packed, but s18_4 is not.
    Pcm_IsPacked = (1 << 3),
    //! Set if sample bit size is multiple of 8.
    //! E.g. s16 and s18_4 are aligned, but s18 is not.
    Pcm_IsAligned = (1 << 4),

    //! Set if format has same endian as CPU.
    //! E.g. s16_le is native if CPU is little-endian,
    //! and s16 is always native.
    Pcm_IsNative = (1 << 5),
    //! Set if format is little-endian.
    //! E.g. s16 is little-endian if CPU is little-endian,
    //! and s16_le is always little-endian.
    Pcm_IsLittle = (1 << 6),
    //! Set if format is big-endian.
    //! E.g. s16 is big-endian if CPU is big-endian,
    //! and s16_be is always big-endian.
    Pcm_IsBig = (1 << 7),
};

//! PCM format meta-information.
struct PcmTraits {
    //! Numeric identifier.
    PcmSubformat id;

    //! String name.
    const char* name;

    //! Flags.
    unsigned flags;

    //! Number of stored bits per sample in binary form.
    size_t bit_width;

    //! Number of significant bits per sample.
    size_t bit_depth;

    //! Same format, but with explicit _Be or _Le suffix.
    //! If format is default-endian, suffix is added based on current CPU,
    //! otherwise this field is same as original format.
    PcmSubformat portable_alias;

    //! Same format, but without explicit _Be or _Le suffix if possible.
    //! If format is native-endian, suffix is removed, otherwise this field
    //! is same as original format.
    PcmSubformat native_alias;

    //! Same format but with removed _Be or _Le suffix.
    //! May be equal to original format.
    PcmSubformat default_variant;

    //! Same format but with _Le suffix.
    //! May be equal to original format.
    PcmSubformat be_variant;

    //! Same format but with _Be suffix.
    //! May be equal to original format.
    PcmSubformat le_variant;

    //! Initialize invalid traits.
    PcmTraits()
        : id(PcmSubformat_Invalid)
        , name(NULL)
        , flags(0)
        , bit_width(0)
        , bit_depth(0)
        , portable_alias(PcmSubformat_Invalid)
        , native_alias(PcmSubformat_Invalid)
        , default_variant(PcmSubformat_Invalid)
        , be_variant(PcmSubformat_Invalid)
        , le_variant(PcmSubformat_Invalid) {
    }

    //! Check if all given flags are set.
    bool has_flags(unsigned mask) const {
        return (flags & mask) == mask;
    }
};

//! PCM mapping function.
typedef void (*PcmMapFn)(const uint8_t* in_data,
                         size_t& in_bit_off,
                         uint8_t* out_data,
                         size_t& out_bit_off,
                         size_t n_samples);

//! Get mapping function for given PCM format pair.
PcmMapFn pcm_subformat_mapfn(PcmSubformat in_format, PcmSubformat out_format);

//! Get format traits for given PCM format.
PcmTraits pcm_subformat_traits(PcmSubformat format);

//! Get string name of PCM format.
const char* pcm_subformat_to_str(PcmSubformat format);

//! Get PCM format from string name.
PcmSubformat pcm_subformat_from_str(const char* str);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_SUBFORMAT_H_
