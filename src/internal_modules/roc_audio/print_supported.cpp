/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/print_supported.h"
#include "roc_audio/channel_defs.h"
#include "roc_audio/channel_tables.h"
#include "roc_audio/pcm_format.h"
#include "roc_audio/sample_format.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/printer.h"

namespace roc {
namespace audio {

namespace {

void print_pcm_formats(core::Printer& prn) {
    PcmTraits prev_traits, curr_traits;

    for (int n = 0; n < PcmFormat_Max; n++) {
        const PcmFormat fmt = (PcmFormat)n;

        curr_traits = pcm_format_traits(fmt);
        if (!curr_traits.is_valid) {
            continue;
        }

        if (prev_traits.bit_depth != curr_traits.bit_depth
            || prev_traits.bit_width != curr_traits.bit_width) {
            if (curr_traits.bit_width % 8 == 0) {
                prn.writef("\n  %2d bit (%d byte)    ", (int)curr_traits.bit_depth,
                           (int)curr_traits.bit_width / 8);
            } else {
                prn.writef("\n  %d bit (%.2f byte) ", (int)curr_traits.bit_depth,
                           (double)curr_traits.bit_width / 8.);
            }
        }

        prev_traits = curr_traits;

        prn.writef(" %s", pcm_format_to_str(fmt));
    }
}

void print_channel_masks(core::Printer& prn) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(ChanMaskNames); i++) {
        const ChannelMask ch_mask = ChanMaskNames[i].mask;

        prn.writef("  %-13s  (", channel_mask_to_str(ch_mask));

        bool first = true;

        for (int ch = 0; ch < ChanPos_Max; ch++) {
            if (ch_mask & (1 << ch)) {
                if (!first) {
                    prn.writef(" ");
                }
                first = false;
                prn.writef("%s", channel_pos_to_str((ChannelPosition)ch));
            }
        }

        prn.writef(")\n");
    }
}

void print_channel_names(core::Printer& prn) {
    prn.writef("  front      FL FR FC\n");
    prn.writef("  side       SL SR\n");
    prn.writef("  back       BL BR BC\n");
    prn.writef("  top front  TFL TFR\n");
    prn.writef("  top mid    TML TMR\n");
    prn.writef("  top back   TBL TBR\n");
    prn.writef("  low freq   LFE\n");
}

} // namespace

bool print_supported() {
    core::Printer prn;

    prn.writef("\nsupported pcm formats:");
    print_pcm_formats(prn);

    prn.writef("\npre-defined channel layouts:\n");
    print_channel_masks(prn);

    prn.writef("\n\npre-defined channel names:\n");
    print_channel_names(prn);

    return true;
}

} // namespace audio
} // namespace roc
