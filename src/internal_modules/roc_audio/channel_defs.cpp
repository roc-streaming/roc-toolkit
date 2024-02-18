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
#include "roc_core/panic.h"

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
    if (order >= 0 && order < (int)ROC_ARRAY_SIZE(ChanOrderTables)) {
        return ChanOrderTables[order].name;
    }

    return NULL;
}

const char* channel_pos_to_str(ChannelPosition pos) {
    if (pos >= 0 && pos < (int)ROC_ARRAY_SIZE(ChanPositionNames)) {
        return ChanPositionNames[pos].name;
    }

    return NULL;
}

const char* channel_mask_to_str(ChannelMask mask) {
    for (size_t i = 0; i < (int)ROC_ARRAY_SIZE(ChanMaskNames); i++) {
        if (ChanMaskNames[i].mask == mask) {
            return ChanMaskNames[i].name;
        }
    }

    return NULL;
}

} // namespace audio
} // namespace roc
