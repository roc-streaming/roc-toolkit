/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_defs.h"
#include "roc_audio/channel_tables.h"
#include "roc_core/macro_helpers.h"

namespace roc {
namespace audio {

const char* channel_layout_to_str(ChannelLayout layout) {
    switch (layout) {
    case ChanLayout_None:
        return "none";

    case ChanLayout_Surround:
        return "surround";

    case ChanLayout_Multitrack:
        return "multitrack";
    }

    return "?";
}

const char* channel_order_to_str(ChannelOrder order) {
    switch (order) {
    case ChanOrder_None:
        return "none";

    case ChanOrder_Smpte:
        return "smpte";

    case ChanOrder_Alsa:
        return "alsa";

    case ChanOrder_Max:
        break;
    }

    return "?";
}

const char* channel_pos_to_str(ChannelPosition pos) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(ChanPositionNames); i++) {
        if (ChanPositionNames[i].pos == pos) {
            return ChanPositionNames[i].name;
        }
    }

    return NULL;
}

ChannelPosition channel_pos_from_str(const char* name) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(ChanPositionNames); i++) {
        if (strcmp(ChanPositionNames[i].name, name) == 0) {
            return ChanPositionNames[i].pos;
        }
    }

    return ChanPos_Max;
}

const char* channel_mask_to_str(ChannelMask mask) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(ChanMaskNames); i++) {
        if (ChanMaskNames[i].mask == mask) {
            return ChanMaskNames[i].name;
        }
    }

    return NULL;
}

ChannelMask channel_mask_from_str(const char* name) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(ChanMaskNames); i++) {
        if (strcmp(ChanMaskNames[i].name, name) == 0) {
            return ChanMaskNames[i].mask;
        }
    }

    return 0;
}

} // namespace audio
} // namespace roc
