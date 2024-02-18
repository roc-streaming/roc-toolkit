/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper_matrix.h"
#include "roc_audio/channel_defs.h"
#include "roc_audio/channel_set_to_str.h"
#include "roc_core/printer.h"

namespace roc {
namespace audio {

ChannelMapperMatrix::ChannelMapperMatrix() {
    memset(index_matrix_, 0, sizeof(index_matrix_));
}

void ChannelMapperMatrix::build(const ChannelSet& in_chans, const ChannelSet& out_chans) {
    // Build mapping of logical channel positions to physical index in frame,
    // based on channel order tables.
    IndexMap in_index_map;
    build_index_mapping_(in_index_map, in_chans);

    IndexMap out_index_map;
    build_index_mapping_(out_index_map, out_chans);

    // Build mapping matrix from input to output logical channel sets, based on
    // downmixing/upmixing tables. Note that instead of original input/output
    // channel sets, we use channel sets from index maps, which are the same
    // but channels that are not supported by particular channel order are removed.
    ChannelMap chan_map;
    if (!build_channel_mapping_(chan_map, in_index_map.enabled_chans,
                                out_index_map.enabled_chans)) {
        fill_fallback_mapping_(chan_map, in_index_map.enabled_chans,
                               out_index_map.enabled_chans);
    }

    // Build final matrix that combines downmixing/upmixing and reordering.
    populate_index_matrix_(in_index_map, out_index_map, chan_map);

    // Print matrix to console.
    if (core::Logger::instance().get_level() >= LogTrace) {
        print_index_matrix_(in_index_map, out_index_map, chan_map);
    }
}

void ChannelMapperMatrix::build_index_mapping_(IndexMap& result_map,
                                               const ChannelSet& ch_set) {
    const ChannelOrderTable& order_table = ChanOrderTables[ch_set.order()];

    size_t ch_index = 0; // Physical index of channel in frame.
    size_t ord_n = 0;

    for (;;) {
        // Logical channel position.
        const ChannelPosition ch = order_table.chans[ord_n++];
        if (ch == ChanPos_Max) {
            // Last channel in table.
            break;
        }

        if (ch_set.has_channel(ch)) {
            result_map.enabled_chans.toggle_channel(ch, true);
            result_map.chan_2_index[ch] = ch_index;
            result_map.index_2_chan[ch_index] = ch;
            ch_index++;
        }
    }

    while (ch_index < ChanPos_Max) {
        result_map.index_2_chan[ch_index] = ChanPos_Max;
        ch_index++;
    }
}

bool ChannelMapperMatrix::build_channel_mapping_(ChannelMap& result_map,
                                                 const ChannelSet& in_chans,
                                                 const ChannelSet& out_chans) {
    if (in_chans == out_chans) {
        // Nothing to map.
        return false;
    }

    // Select channel mapping table that covers conversion from input to output mask.
    bool map_reversed = false;
    const ChannelMapTable* map_table = find_table_(in_chans, out_chans, map_reversed);
    if (!map_table) {
        return false;
    }

    roc_log(LogDebug,
            "channel mapper matrix:"
            " selected mapping table: in_chans=%s out_chans=%s table=[%s] is_reverse=%d",
            channel_set_to_str(in_chans).c_str(), channel_set_to_str(out_chans).c_str(),
            map_table->name, (int)map_reversed);

    // Fill mapping matrix based on rules from table.
    fill_mapping_from_table_(result_map, *map_table, map_reversed, in_chans, out_chans);

    // Normalize matrix.
    normalize_mapping_(result_map);

    return true;
}

const ChannelMapTable* ChannelMapperMatrix::find_table_(const ChannelSet& in_chans,
                                                        const ChannelSet& out_chans,
                                                        bool& map_reversed) {
    // Find first downmixing table that covers both output and input masks.
    // We assume that tables are sorted from "smaller" to "larger" channel masks.
    for (size_t n = 0; n < ROC_ARRAY_SIZE(ChanMapTables); n++) {
        const ChannelMapTable& tbl = ChanMapTables[n];

        // Table covers our pair of masks directly,
        // which means that we're downmixing.
        if (out_chans.is_subset(tbl.out_mask) && in_chans.is_subset(tbl.in_mask)) {
            map_reversed = false;
            return &tbl;
        }

        // Table covers our pair of masks after being reversed,
        // which means that we're upmixing.
        if (in_chans.is_subset(tbl.out_mask) && out_chans.is_subset(tbl.in_mask)) {
            map_reversed = true;
            return &tbl;
        }
    }

    return NULL;
}

void ChannelMapperMatrix::fill_mapping_from_table_(ChannelMap& result_map,
                                                   const ChannelMapTable& map_table,
                                                   bool map_reversed,
                                                   const ChannelSet& in_chans,
                                                   const ChannelSet& out_chans) {
    // Build matrix based on rules from table.
    for (size_t n = 0; n < ROC_ARRAY_SIZE(map_table.rules); n++) {
        const ChannelMapRule& rule = map_table.rules[n];
        if (rule.coeff == 0.f) {
            // Last rule in table.
            break;
        }

        ChannelPosition out_ch = ChanPos_Max;
        ChannelPosition in_ch = ChanPos_Max;
        sample_t coeff = 0.f;

        if (!map_reversed) {
            out_ch = rule.out_ch;
            in_ch = rule.in_ch;
            coeff = rule.coeff;
        } else {
            out_ch = rule.in_ch;
            in_ch = rule.out_ch;
            coeff = 1.f / rule.coeff;
        }

        if (in_chans.has_channel(in_ch) && out_chans.has_channel(out_ch)) {
            result_map.chan_matrix[out_ch][in_ch] = coeff;
        }
    }
}

void ChannelMapperMatrix::fill_fallback_mapping_(ChannelMap& result_map,
                                                 const ChannelSet& in_chans,
                                                 const ChannelSet& out_chans) {
    roc_log(LogDebug,
            "channel mapper matrix:"
            " selected mapping table: in_chans=%s out_chans=%s table=[diagonal]",
            channel_set_to_str(in_chans).c_str(), channel_set_to_str(out_chans).c_str());

    // There is no selected channel mapping table, which can happen if no mapping is
    // needed or no matching table is found. In this case we fall back to diagonal
    // mapping matrix, where each channel is mapped just to itself.
    for (size_t ch = 0; ch < ChanPos_Max; ch++) {
        result_map.chan_matrix[ch][ch] = 1.0f;
    }
}

void ChannelMapperMatrix::normalize_mapping_(ChannelMap& chan_map) {
    for (size_t out_ch = 0; out_ch < ChanPos_Max; out_ch++) {
        sample_t coeff_sum = 0;

        for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
            coeff_sum += chan_map.chan_matrix[out_ch][in_ch];
        }

        if (coeff_sum == 0.f) {
            continue;
        }

        for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
            chan_map.chan_matrix[out_ch][in_ch] /= coeff_sum;
        }
    }
}

void ChannelMapperMatrix::populate_index_matrix_(const IndexMap& in_index_map,
                                                 const IndexMap& out_index_map,
                                                 const ChannelMap& chan_map) {
    // Combine two index mappings (for channel reordering) and channel mapping (for
    // downmixing/upmixing) into a single matrix that performs both transformations.
    for (size_t out_ch = 0; out_ch < ChanPos_Max; out_ch++) {
        if (!out_index_map.enabled_chans.has_channel(out_ch)) {
            continue;
        }

        for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
            if (!in_index_map.enabled_chans.has_channel(in_ch)) {
                continue;
            }

            const size_t out_index = out_index_map.chan_2_index[out_ch];
            const size_t in_index = in_index_map.chan_2_index[in_ch];

            roc_panic_if_not(out_index < ChanPos_Max);
            roc_panic_if_not(in_index < ChanPos_Max);

            index_matrix_[out_index][in_index] = chan_map.chan_matrix[out_ch][in_ch];
        }
    }
}

void ChannelMapperMatrix::print_index_matrix_(const IndexMap& in_index_map,
                                              const IndexMap& out_index_map,
                                              const ChannelMap& chan_map) {
    core::Printer prn;

    prn.writef("     ");

    for (size_t in_index = 0; in_index < ChanPos_Max; in_index++) {
        if (in_index_map.index_2_chan[in_index] == ChanPos_Max) {
            break;
        }

        const ChannelPosition in_ch = in_index_map.index_2_chan[in_index];

        prn.writef(" %6s", channel_pos_to_str(in_ch));
    }

    prn.writef("\n");

    for (size_t out_index = 0; out_index < ChanPos_Max; out_index++) {
        if (out_index_map.index_2_chan[out_index] == ChanPos_Max) {
            break;
        }

        const ChannelPosition out_ch = out_index_map.index_2_chan[out_index];

        prn.writef(" %3s ", channel_pos_to_str(out_ch));

        for (size_t in_index = 0; in_index < ChanPos_Max; in_index++) {
            if (in_index_map.index_2_chan[in_index] == ChanPos_Max) {
                break;
            }

            if (index_matrix_[out_index][in_index] == 0) {
                prn.writef("  .....");
            } else {
                prn.writef("  %.3f", (double)index_matrix_[out_index][in_index]);
            }
        }

        prn.writef("\n");
    }
}

} // namespace audio
} // namespace roc
