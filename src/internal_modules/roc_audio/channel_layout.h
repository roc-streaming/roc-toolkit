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
    ChanLayout_Invalid,

    //! Multi-channel mono / stereo / surround sound.
    //! @remarks
    //!  The meaning of channel index is defined by ChannelPosition enum.
    ChanLayout_Surround,

    //! Multi-channel multi-track sound.
    //! @remarks
    //!  There is no special meaning of channels, they are considered to
    //!  be independent tracks.
    ChanLayout_Multitrack
};

//! Channel position.
//! @remarks
//!  Should be used with ChannelLayout_Surround.
//!  Defines meaning of channel indicies for mono / stereo / surround sound.
//! @note
//!  Despide mono, stereo, and 3.x technically are not surround layouts, in
//!  the code base they are considered a special case of surround.
enum ChannelPosition {
    //! @name Front speakers.
    //! @remarks
    //!  Placed in the front of the user.
    // @{
    ChanPos_FrontLeft,   //!< Front left (FL).
    ChanPos_FrontCenter, //!< Front center (FC).
    ChanPos_FrontRight,  //!< Front right (FR).
    // @}

    //! @name Surround speakers.
    //! @remarks
    //!  Placed behind the user (in surround 4.x, 5.x, or 6.x),
    //!  or on the sides (in surround 7.x).
    //!  Also known as "mid" or "side" speakers.
    // @{
    ChanPos_SurroundLeft,   //!< Surround left (SL).
    ChanPos_SurroundCenter, //!< Surround center (SC).
    ChanPos_SurroundRight,  //!< Surround right (SR).
    // @}

    //! @name Back speakers.
    //! @remarks
    //!  Placed behind the user (in surround 7.x).
    //!  Also known as "rear" speakers.
    // @{
    ChanPos_BackLeft,  //!< Back left (BL).
    ChanPos_BackRight, //!< Back right (BR).
    // @}

    //! @name Top speakers.
    //! @remarks
    //!  Placed above the user (in surround x.1.2 and x.1.4).
    //!  Also known as "height" or "overhead" speakers.
    // @{
    ChanPos_TopFrontLeft,  //!< Top front left (TFL).
    ChanPos_TopFrontRight, //!< Top front right (TFR).

    ChanPos_TopMidLeft,  //!< Top middle left (TML).
    ChanPos_TopMidRight, //!< Top middle right (TMR).

    ChanPos_TopBackLeft,  //!< Top rear left (TBL).
    ChanPos_TopBackRight, //!< Top rear right (TBR).
    // @}

    //! Low frequency speaker (LFE).
    //! @remarks
    //!  Placed anywhere.
    //!  Also known as "subwoofer" or "SW" speaker.
    ChanPos_LowFrequency,

    //! Maximum channel number.
    ChanPos_Max
};

//! Channel mask.
//! @remarks
//!  Used to construct short channel sets (up to 32 channels)
//!  for ChanLayout_Surround layout.
typedef uint32_t ChannelMask;

//! Mono.
//! Mask: FC.
//! @code
//!  +------------------+
//!  |        FC        |
//!  |       user       |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_Mono = //
    (1 << ChanPos_FrontCenter);

//! Stereo.
//! Mask: FL, FR.
//! @code
//!  +------------------+
//!  |  FL          FR  |
//!  |       user       |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_Stereo =
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontRight);

//! Stereo + subwoofer.
//! Mask: FL, FR | LFE.
//!  +------------------+
//!  |  FL          FR  |
//!  |       user       |
//!  |             LFE  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_2_1 = //
    ChanMask_Surround_Stereo                     //
    | (1 << ChanPos_LowFrequency);

//! Three front channels.
//! Mask: FL, FC, FR.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |       user       |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_3_0 =
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontCenter) | (1 << ChanPos_FrontRight);

//! Three front channels + subwoofer.
//! Mask: FL, FC, FR | LFE.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |       user       |
//!  |             LFE  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_3_1 = //
    ChanMask_Surround_3_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 4.0.
//! Mask: FL, FR, SL, SR.
//! @code
//!  +------------------+
//!  |  FL          FR  |
//!  |       user       |
//!  |  SL          SR  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_4_0 =         //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontRight) //
    | (1 << ChanPos_SurroundLeft) | (1 << ChanPos_SurroundRight);

//! Surround 4.1.
//! Mask: FL, FR, SL, SR | LFE.
//! @code
//!  +------------------+
//!  |  FL          FR  |
//!  |       user       |
//!  |             LFE  |
//!  |  SL          SR  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_4_1 = //
    ChanMask_Surround_4_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 5.0.
//! Mask: FL, FC, FR, SL, SR.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |       user       |
//!  |  SL          SR  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_0 =                                      //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontCenter) | (1 << ChanPos_FrontRight) //
    | (1 << ChanPos_SurroundLeft) | (1 << ChanPos_SurroundRight);

//! Surround 5.1.
//! Mask: FL, FC, FR, SL, SR | LFE.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |       user       |
//!  |             LFE  |
//!  |  SL          SR  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1 = //
    ChanMask_Surround_5_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 5.1.2.
//! Mask: FL, FC, FR, SL, SR | LFE | TML, TMR.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |                  |
//!  |   TML user TMR   |
//!  |             LFE  |
//!  |  SL          SR  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1_2 = //
    ChanMask_Surround_5_0                          //
    | (1 << ChanPos_LowFrequency)                  //
    | (1 << ChanPos_TopMidLeft) | (1 << ChanPos_TopMidRight);

//! Surround 5.1.4.
//! Mask: FL, FC, FR, SL, SR | LFE | TFL, TFR, TBL, TBR.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |   TFL      TFR   |
//!  |       user       |
//!  |             LFE  |
//!  |   TBL      TBR   |
//!  |  SL          SR  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1_4 = //
    ChanMask_Surround_5_0                          //
    | (1 << ChanPos_LowFrequency)                  //
    | (1 << ChanPos_TopFrontLeft) | (1 << ChanPos_TopFrontRight)
    | (1 << ChanPos_TopBackLeft) | (1 << ChanPos_TopBackRight);

//! Surround 6.0.
//! Mask: FL, FC, FR, SL, SC, SR.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |       user       |
//!  |  SL    SC    SR  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_6_0 =                                      //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontCenter) | (1 << ChanPos_FrontRight) //
    | (1 << ChanPos_SurroundLeft) | (1 << ChanPos_SurroundCenter)
    | (1 << ChanPos_SurroundRight);

//! Surround 6.1.
//! Mask: FL, FC, FR, SL, SC, SR | LFE.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |       user       |
//!  |             LFE  |
//!  |  SL    SC    SR  |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_6_1 = //
    ChanMask_Surround_6_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 7.0.
//! Mask: FL, FC, FR, SL, SR, BL, BR.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |  SL   user   SR  |
//!  |    BL      BR    |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_0 =                                      //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontCenter) | (1 << ChanPos_FrontRight) //
    | (1 << ChanPos_SurroundLeft) | (1 << ChanPos_SurroundRight)                      //
    | (1 << ChanPos_BackLeft) | (1 << ChanPos_BackRight);

//! Surround 7.1.
//! Mask: FL, FC, FR, SL, SR, BL, BR | LFE.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |  SL   user   SR  |
//!  |             LFE  |
//!  |    BL      BR    |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1 = //
    ChanMask_Surround_7_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 7.1.2.
//! Mask: FL, FC, FR, SL, SR, BL, BR | LFE | TML, TMR.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |                  |
//!  |    TML     TMR   |
//!  |  SL   user   SR  |
//!  |             LFE  |
//!  |    BL      BR    |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1_2 = //
    ChanMask_Surround_7_0                          //
    | (1 << ChanPos_LowFrequency)                  //
    | (1 << ChanPos_TopMidLeft) | (1 << ChanPos_TopMidRight);

//! Surround 7.1.4.
//! Mask: FL, FC, FR, SL, SR, BL, BR | LFE | TFL, TFR, TBL, TBR.
//! @code
//!  +------------------+
//!  |  FL    FC    FR  |
//!  |   TFL      TFR   |
//!  |  SL   user   SR  |
//!  |             LFE  |
//!  |   TBL      TBR   |
//!  |    BL      BR    |
//!  +------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1_4 = //
    ChanMask_Surround_7_0                          //
    | (1 << ChanPos_LowFrequency)                  //
    | (1 << ChanPos_TopFrontLeft) | (1 << ChanPos_TopFrontRight)
    | (1 << ChanPos_TopBackLeft) | (1 << ChanPos_TopBackRight);

//! Get string name of channel layout.
const char* channel_layout_to_str(ChannelLayout);

//! Get string name of channel position.
const char* channel_position_to_str(ChannelPosition);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_LAYOUT_H_
