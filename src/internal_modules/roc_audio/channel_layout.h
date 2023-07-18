/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_layout.h
//! @brief Channel layout and numbers.

#ifndef ROC_AUDIO_CHANNEL_LAYOUT_H_
#define ROC_AUDIO_CHANNEL_LAYOUT_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! Channel layout.
//! Defines meaning of channels in ChannelSet.
//! ChannelMapper uses channel layout to decide how to perform mapping.
enum ChannelLayout {
    //! Invalid value.
    //! @remarks
    //!  Indicates that channel layout was not set.
    ChannelLayout_Invalid,

    //! Single channel monophonic sound.
    //! @remarks
    //!  Only one channel, with index 0, should be used.
    ChannelLayout_Mono,

    //! Multi-channel stereo or surround sound.
    //! @remarks
    //!  Multiple channels are allowed.
    //!  The meaning of channel index is defined by ChannelPosition enum.
    ChannelLayout_Surround,

    //! Multi-channel multi-track sound.
    //! @remarks
    //!  Multiple channels are allowed.
    //!  There is no special meaning of channels, they are considered to
    //!  be independent tracks.
    ChannelLayout_Multitrack
};

//! Channel position.
//! @remarks
//!  Should be used with ChannelLayout_Surround.
//!  Defines meaning of channel indicies for stereo and surround sound.
enum ChannelPosition {
    //! Left channel.
    ChannelPos_Left = 0,

    //! Right channel.
    ChannelPos_Right = 1
};

//! Channel mask.
//! @remarks
//!  Used to construct short channel sets (up to 32 channels).
typedef uint32_t ChannelMask;

//! Channel mask for mono.
//! @remarks
//!  Should be used with ChannelLayout_Mono.
//!  Defines one channel.
static const ChannelMask ChannelMask_Mono = 0x1;

//! Channel mask for stereo.
//! @remarks
//!  Should be used with ChannelLayout_Surround.
//!  Defines two channels: L, R.
static const ChannelMask ChannelMask_Stereo =
    (1 << ChannelPos_Left) | (1 << ChannelPos_Right);

//! Get string name of channel layout.
const char* channel_layout_to_str(ChannelLayout);

//! Get string name of channel position.
const char* channel_position_to_str(ChannelPosition);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_LAYOUT_H_
