/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/format.h
//! @brief Audio format.

#ifndef ROC_AUDIO_FORMAT_H_
#define ROC_AUDIO_FORMAT_H_

#include "roc_audio/pcm_subformat.h"

namespace roc {
namespace audio {

//! Audio format.
//! Defines representation of samples in memory.
//! Does not define sample depth, rate and channel set.
enum Format {
    //! Invalid format.
    Format_Invalid,

    //! Interleaved PCM format.
    //! @note
    //!  Can be used for network packets, devices, files.
    //! @note
    //!  This format requires sub-format of type PcmSubformat, which defines
    //!  sample type, width, and endian.
    Format_Pcm,

    //! WAV file.
    //! @note
    //!  Can be used for files.
    //! @note
    //!  This format allows optional sub-format of type PcmSubformat, which defines
    //!  sample type, width, and endian. However, not every PCM sub-format is
    //!  supported. If sub-format is omitted, default sub-format is used.
    Format_Wav,

    //! Custom opaque format.
    //! @remarks
    //!  Used to specify custom format for file or device via its string
    //!  name, when we don't have and don't need enum value for it.
    Format_Custom,

    //! Maximum enum value.
    Format_Max
};

//! Audio format flags.
enum FormatFlags {
    //! Format can be used for network packets.
    Format_SupportsNetwork = (1 << 0),

    //! Format can be used for audio devices.
    Format_SupportsDevices = (1 << 1),

    //! Format can be used for audio files.
    Format_SupportsFiles = (1 << 2)
};

//! Audio format meta-information.
struct FormatTraits {
    //! Numeric identifier.
    Format id;

    //! String name.
    const char* name;

    //! Flags.
    unsigned flags;

    //! Check if all given flags are set.
    bool has_flags(unsigned mask) const {
        return (flags & mask) == mask;
    }
};

//! Get format traits.
FormatTraits format_traits(Format format);

//! Get string name of audio format.
const char* format_to_str(Format format);

//! Get audio format from string name.
Format format_from_str(const char* str);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_FORMAT_H_
