/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_set.h"

namespace roc {
namespace audio {

void format_channel_set(const ChannelSet& ch_set, core::StringBuilder& bld) {
    bld.append_str("<");
    bld.append_str(channel_layout_to_str(ch_set.layout()));

    if (ch_set.order() != ChanOrder_None) {
        bld.append_str(" ");
        bld.append_str(channel_order_to_str(ch_set.order()));
    }

    bld.append_str(" ");
    bld.append_uint(ch_set.num_channels(), 10);

    if (ch_set.num_channels() == 0) {
        bld.append_str(" none");
    } else if (ch_set.layout() == ChanLayout_Surround) {
        bld.append_str(" ");

        for (size_t ch = ch_set.first_channel(); ch <= ch_set.last_channel(); ch++) {
            if (!ch_set.has_channel(ch)) {
                continue;
            }
            if (ch != ch_set.first_channel()) {
                bld.append_str(",");
            }
            bld.append_str(channel_pos_to_str((ChannelPosition)ch));
        }
    } else {
        bld.append_str(" 0x");

        size_t last_byte = 0;

        for (size_t n = 0; n < ch_set.num_bytes(); n++) {
            if (ch_set.byte_at(n) != 0) {
                last_byte = n;
            }
        }

        size_t n = last_byte;
        do {
            const uint8_t byte = ch_set.byte_at(n);

            const uint8_t lo = (byte & 0xf);
            const uint8_t hi = ((byte >> 4) & 0xf);

            if (hi != 0 || n != last_byte) {
                bld.append_uint(hi, 16);
            }
            bld.append_uint(lo, 16);
        } while (n-- != 0);
    }

    bld.append_str(">");
}

} // namespace audio
} // namespace roc
