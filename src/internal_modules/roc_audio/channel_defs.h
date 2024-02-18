/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_defs.h
//! @brief Channel layout, order, and positions.

#ifndef ROC_AUDIO_CHANNEL_DEFS_H_
#define ROC_AUDIO_CHANNEL_DEFS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! Channel layout.
//! Defines meaning of channels in ChannelSet.
//! ChannelMapper uses channel layout to decide how to perform mapping.
enum ChannelLayout {
    //! Channel layout is not set.
    //! @remarks
    //!  This is never valid and indicates that ChannelSet is not fully initialized.
    ChanLayout_None,

    //! Multi-channel mono / stereo / surround sound.
    //! @remarks
    //!  The meaning of channel index is defined by ChannelPosition enum.
    //!  Channels are mapped according to their position in space, e.g. if
    //!  top-left channel is missing, it can be mixed from front-left and
    //!  side-left channels.
    ChanLayout_Surround,

    //! Multi-channel multi-track sound.
    //! @remarks
    //!  There is no special meaning of channels, they are considered to be
    //!  independent tracks. Channels are mapped according to their numbers;
    //!  channel N is mapped to channel N and nothing else.
    ChanLayout_Multitrack
};

//! Surround channel order.
//! @remarks
//!  Should be used with ChannelLayout_Surround.
//!  Defines order in which channels from ChannelSet are (de)serialized.
enum ChannelOrder {
    //! Channel order is not set.
    //! @remarks
    //!  For ChanLayout_Surround, this is never valid and indicates that ChannelSet
    //!  is not fully initialized. For ChanLayout_Multitrack, in contrast, this is
    //!  the only valid value.
    ChanOrder_None,

    //! ITU/SMPTE channel order.
    //! Order: FL, FR, FC, LFE, BL, BR, BC, SL, SR, TFL, TFR, TBL, TBR, TML, TMR.
    //! @remarks
    //!  This order is actually a superset of what is defined by SMPTE, but when
    //!  filtered by actual masks like 5.1 or 7.1, it produces orderings equal
    //!  to what is defined in the standard.
    //!  When used with masks 2.x - 5.x (but not 6.x), it is also compatible with
    //!  the channel order from AIFF-C, which is used by default in RTP/AVP, as
    //!  defined in RFC 3551.
    ChanOrder_Smpte,

    //! ALSA channel order.
    //! Order: FL, FR, BL, BR, FC, LFE, SL, SR, BC.
    //! @remarks
    //!  This order is used by ALSA hardware devices.
    //!  ALSA supports only 9 channels.
    ChanOrder_Alsa,

    //! Maximum value of enum.
    ChanOrder_Max
};

//! Surround channel position.
//! @remarks
//!  Should be used with ChannelLayout_Surround.
//!  Defines meaning of channel indices for mono / stereo / surround sound.
//! @note
//!  Despite mono, stereo, and 3.x technically are not surround layouts, in
//!  the code base they are considered a special case of surround.
enum ChannelPosition {
    //! @name Front speakers.
    //! @remarks
    //!  Placed in the front of the user.
    //!  FLC and FRC are typically used for 3-channel center speaker.
    // @{
    ChanPos_FrontLeft,          //!< Front left (FL).
    ChanPos_FrontLeftOfCenter,  //!< Front left of center (FLC).
    ChanPos_FrontCenter,        //!< Front center (FC).
    ChanPos_FrontRightOfCenter, //!< Front right of center (FRC).
    ChanPos_FrontRight,         //!< Front right (FR).
    // @}

    //! @name Surround speakers.
    //! @remarks
    //!  Placed on the sides of the user (in surround 7.x).
    //!  Also known as "mid" speakers.
    // @{
    ChanPos_SideLeft,  //!< Side left (SL).
    ChanPos_SideRight, //!< Side right (SR).
    // @}

    //! @name Back speakers.
    //! @remarks
    //!  Placed behind the user.
    //!  Also known as "rear" speakers.
    // @{
    ChanPos_BackLeft,   //!< Back left (BL).
    ChanPos_BackCenter, //!< Back center (BC).
    ChanPos_BackRight,  //!< Back right (BR).
    // @}

    //! @name Top speakers.
    //! @remarks
    //!  Placed above the user.
    //!  Also known as "height" or "overhead" speakers.
    //!  TFC and TBC are typically used for 3-channel overhead soundbars.
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

    //! Maximum value of enum.
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
//!  +----------------------+
//!  |          FC          |
//!  |         user         |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_Mono = //
    (1 << ChanPos_FrontCenter);

//! Mono + subwoofer.
//! Mask: FC | LFE.
//! @code
//!  +----------------------+
//!  |          FC          |
//!  |         user         |
//!  |              LFE     |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_1_1 = //
    ChanMask_Surround_Mono                       //
    | (1 << ChanPos_LowFrequency);

//! 3-channel center speaker + subwoofer.
//! Mask: FLC, FC, FRC | LFE.
//! @code
//!  +----------------------+
//!  |      FLC|FC|FRC      |
//!  |         user         |
//!  |              LFE     |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_1_1_3c = //
    ChanMask_Surround_1_1 |                         //
    (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Stereo.
//! Mask: FL, FR.
//! @code
//!  +----------------------+
//!  |  FL             FR   |
//!  |         user         |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_Stereo = //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontRight);

//! Stereo + subwoofer.
//! Mask: FL, FR | LFE.
//!  +----------------------+
//!  |  FL             FR   |
//!  |         user         |
//!  |              LFE     |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_2_1 = //
    ChanMask_Surround_Stereo                     //
    | (1 << ChanPos_LowFrequency);

//! Three front speakers.
//! Mask: FL, FC, FR.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |         user         |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_3_0 =
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontCenter) | (1 << ChanPos_FrontRight);

//! Three front speakers + subwoofer.
//! Mask: FL, FC, FR | LFE.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |         user         |
//!  |              LFE     |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_3_1 = //
    ChanMask_Surround_3_0                        //
    | (1 << ChanPos_LowFrequency);

//! Three front speakers + subwoofer, with 3-channel center speaker.
//! Mask: FL, FLC, FC, FRC, FR | LFE.
//! @code
//!  +----------------------+
//!  |  FL  FLC|FC|FRC  FR  |
//!  |         user         |
//!  |              LFE     |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_3_1_3c = //
    ChanMask_Surround_3_1                           //
    | (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Surround 4.0.
//! Mask: FL, FR, BL, BR.
//! @code
//!  +----------------------+
//!  |  FL             FR   |
//!  |         user         |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_4_0 =         //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontRight) //
    | (1 << ChanPos_BackLeft) | (1 << ChanPos_BackRight);

//! Surround 4.1.
//! Mask: FL, FR, BL, BR | LFE.
//! @code
//!  +----------------------+
//!  |  FL             FR   |
//!  |         user         |
//!  |              LFE     |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_4_1 = //
    ChanMask_Surround_4_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 5.0.
//! Mask: FL, FC, FR, BL, BR.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |         user         |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_0 =                                      //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontCenter) | (1 << ChanPos_FrontRight) //
    | (1 << ChanPos_BackLeft) | (1 << ChanPos_BackRight);

//! Surround 5.1.
//! Mask: FL, FC, FR, BL, BR | LFE.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |         user         |
//!  |              LFE     |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1 = //
    ChanMask_Surround_5_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 5.1, with 3-channel center speaker.
//! Mask: FL, FLC, FC, FRC, FR, BL, BR | LFE.
//! @code
//!  +----------------------+
//!  |  FL  FLC|FC|FRC  FR  |
//!  |         user         |
//!  |              LFE     |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1_3c = //
    ChanMask_Surround_5_1                           //
    | (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Surround 5.1.2.
//! Mask: FL, FC, FR, BL, BR | LFE | TML, TMR.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |                      |
//!  |    TML  user  TMR    |
//!  |              LFE     |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1_2 = //
    ChanMask_Surround_5_1                          //
    | (1 << ChanPos_TopMidLeft) | (1 << ChanPos_TopMidRight);

//! Surround 5.1.2, with 3-channel center speaker.
//! Mask: FL, FLC, FC, FRC, FR, BL, BR | LFE | TML, TMR.
//! @code
//!  +----------------------+
//!  |  FL  FLC|FC|FRC  FR  |
//!  |                      |
//!  |    TML  user  TMR    |
//!  |              LFE     |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1_2_3c = //
    ChanMask_Surround_5_1_2                           //
    | (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Surround 5.1.4.
//! Mask: FL, FC, FR, BL, BR | LFE | TFL, TFR, TBL, TBR.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |    TFL        TFR    |
//!  |         user         |
//!  |              LFE     |
//!  |    TBL        TBR    |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1_4 = //
    ChanMask_Surround_5_1                          //
    | (1 << ChanPos_TopFrontLeft) | (1 << ChanPos_TopFrontRight)
    | (1 << ChanPos_TopBackLeft) | (1 << ChanPos_TopBackRight);

//! Surround 5.1.4, with 3-channel center speaker.
//! Mask: FL, FLC, FC, FRC, FR, BL, BR | LFE | TFL, TFR, TBL, TBR.
//! @code
//!  +----------------------+
//!  |  FL  FLC|FC|FRC  FR  |
//!  |    TFL        TFR    |
//!  |         user         |
//!  |              LFE     |
//!  |    TBL        TBR    |
//!  |  BL             BR   |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_5_1_4_3c = //
    ChanMask_Surround_5_1_4                           //
    | (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Surround 6.0.
//! Mask: FL, FC, FR, BL, BC, BR.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |         user         |
//!  |  BL      BC      BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_6_0 =                                      //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontCenter) | (1 << ChanPos_FrontRight) //
    | (1 << ChanPos_BackLeft) | (1 << ChanPos_BackCenter) | (1 << ChanPos_BackRight);

//! Surround 6.1.
//! Mask: FL, FC, FR, BL, BC, BR | LFE.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |         user         |
//!  |              LFE     |
//!  |  BL      BC      BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_6_1 = //
    ChanMask_Surround_6_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 6.1, with 3-channel center speaker.
//! Mask: FL, FLC, FC, FRC, FR, BL, BC, BR | LFE.
//! @code
//!  +----------------------+
//!  |  FL  FLC|FC|FRC  FR  |
//!  |         user         |
//!  |              LFE     |
//!  |  BL      BC      BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_6_1_3c = //
    ChanMask_Surround_6_1                           //
    | (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Surround 7.0.
//! Mask: FL, FC, FR, SL, SR, BL, BR.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |   SL    user    SR   |
//!  |  BL              BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_0 =                                      //
    (1 << ChanPos_FrontLeft) | (1 << ChanPos_FrontCenter) | (1 << ChanPos_FrontRight) //
    | (1 << ChanPos_SideLeft) | (1 << ChanPos_SideRight)                              //
    | (1 << ChanPos_BackLeft) | (1 << ChanPos_BackRight);

//! Surround 7.1.
//! Mask: FL, FC, FR, SL, SR, BL, BR | LFE.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |   SL    user    SR   |
//!  |              LFE     |
//!  |  BL              BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1 = //
    ChanMask_Surround_7_0                        //
    | (1 << ChanPos_LowFrequency);

//! Surround 7.1, with 3-channel center speaker.
//! Mask: FL, FLC, FC, FRC, FR, SL, SR, BL, BR | LFE.
//! @code
//!  +----------------------+
//!  |  FL  FLC|FC|FRC  FR  |
//!  |   SL    user    SR   |
//!  |              LFE     |
//!  |  BL              BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1_3c = //
    ChanMask_Surround_7_1                           //
    | (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Surround 7.1.2.
//! Mask: FL, FC, FR, SL, SR, BL, BR | LFE | TML, TMR.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |                      |
//!  |    TML        TMR    |
//!  |   SL    user    SR   |
//!  |              LFE     |
//!  |  BL              BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1_2 = //
    ChanMask_Surround_7_1                          //
    | (1 << ChanPos_TopMidLeft) | (1 << ChanPos_TopMidRight);

//! Surround 7.1.2, with 3-channel center speaker.
//! Mask: FL, FLC, FC, FRC, FR, SL, SR, BL, BR | LFE | TML, TMR.
//! @code
//!  +----------------------+
//!  |  FL  FLC|FC|FRC  FR  |
//!  |                      |
//!  |    TML        TMR    |
//!  |   SL    user    SR   |
//!  |              LFE     |
//!  |  BL              BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1_2_3c = //
    ChanMask_Surround_7_1_2                           //
    | (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Surround 7.1.4.
//! Mask: FL, FC, FR, SL, SR, BL, BR | LFE | TFL, TFR, TBL, TBR.
//! @code
//!  +----------------------+
//!  |  FL      FC      FR  |
//!  |    TFL        TFR    |
//!  |   SL    user    SR   |
//!  |              LFE     |
//!  |    TBL        TBR    |
//!  |  BL              BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1_4 = //
    ChanMask_Surround_7_1                          //
    | (1 << ChanPos_TopFrontLeft) | (1 << ChanPos_TopFrontRight)
    | (1 << ChanPos_TopBackLeft) | (1 << ChanPos_TopBackRight);

//! Surround 7.1.4, with 3-channel center speaker.
//! Mask: FL, FLC, FC, FRC, FR, SL, SR, BL, BR | LFE | TFL, TFR, TBL, TBR.
//! @code
//!  +----------------------+
//!  |  FL  FLC|FC|FRC  FR  |
//!  |    TFL        TFR    |
//!  |   SL    user    SR   |
//!  |              LFE     |
//!  |    TBL        TBR    |
//!  |  BL              BR  |
//!  +----------------------+
//! @endcode
static const ChannelMask ChanMask_Surround_7_1_4_3c = //
    ChanMask_Surround_7_1_4                           //
    | (1 << ChanPos_FrontLeftOfCenter) | (1 << ChanPos_FrontRightOfCenter);

//! Get string name of channel layout.
const char* channel_layout_to_str(ChannelLayout);

//! Get string name of channel order.
const char* channel_order_to_str(ChannelOrder);

//! Get string name from channel position.
const char* channel_pos_to_str(ChannelPosition);

//! Get string name from channel mask.
const char* channel_mask_to_str(ChannelMask);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_DEFS_H_
