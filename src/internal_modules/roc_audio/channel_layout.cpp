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
    case ChannelLayout_Invalid:
        break;

    case ChannelLayout_Mono:
        return "mono";

    case ChannelLayout_Surround:
        return "surround";

    case ChannelLayout_Multitrack:
        return "multitrack";
    }

    return "invalid";
}

const char* channel_position_to_str(ChannelPosition position) {
    switch (position) {
    case ChannelPos_Left:
        return "L";

    case ChannelPos_Right:
        return "R";
    }

    return "?";
}

} // namespace audio
} // namespace roc
