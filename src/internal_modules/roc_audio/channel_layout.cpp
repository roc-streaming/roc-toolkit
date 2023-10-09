/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_layout.h"

namespace roc {
namespace audio {

const char* channel_layout_to_str(ChannelLayout layout) {
    switch (layout) {
    case ChanLayout_Invalid:
        break;

    case ChanLayout_Surround:
        return "surround";

    case ChanLayout_Multitrack:
        return "multitrack";
    }

    return "invalid";
}

const char* channel_position_to_str(ChannelPosition position) {
    switch (position) {
    case ChanPos_FrontLeft:
        return "FL";

    case ChanPos_FrontCenter:
        return "FC";

    case ChanPos_FrontRight:
        return "FR";

    case ChanPos_SideLeft:
        return "SL";

    case ChanPos_SideRight:
        return "SR";

    case ChanPos_BackLeft:
        return "BL";

    case ChanPos_BackCenter:
        return "BC";

    case ChanPos_BackRight:
        return "BR";

    case ChanPos_TopFrontLeft:
        return "TFL";

    case ChanPos_TopFrontRight:
        return "TFR";

    case ChanPos_TopMidLeft:
        return "TML";

    case ChanPos_TopMidRight:
        return "TMR";

    case ChanPos_TopBackLeft:
        return "TBL";

    case ChanPos_TopBackRight:
        return "TBR";

    case ChanPos_LowFrequency:
        return "LFE";

    case ChanPos_Max:
        break;
    }

    return "?";
}

} // namespace audio
} // namespace roc
