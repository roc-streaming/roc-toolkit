/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper_matrix.h"
#include "roc_audio/channel_set_to_str.h"

namespace roc {
namespace audio {

ChannelMapperMatrix::ChannelMapperMatrix() {
    memset(matrix_, 0, sizeof(matrix_));
}

void ChannelMapperMatrix::build(const ChannelSet& in_chans, const ChannelSet& out_chans) {
    // Build mapping of logical channel position to physical index in frame,
    // based on channel order tables.
    IndexMap out_mapping;
    build_index_mapping_(out_mapping, out_chans);

    IndexMap in_mapping;
    build_index_mapping_(in_mapping, in_chans);

    // Select channel mapping table that covers conversion from
    // input to output channel mask.
    const ChannelMapTable* map_table = NULL;
    bool map_reversed = false;

    if (out_mapping.index_set != in_mapping.index_set) {
        map_table = select_mapping_table_(out_mapping, in_mapping, map_reversed);
    }

    if (map_table) {
        roc_log(
            LogDebug,
            "channel mapper matrix:"
            " selected mapping table: in_chans=%s out_chans=%s table=[%s] is_reverse=%d",
            channel_set_to_str(in_chans).c_str(), channel_set_to_str(out_chans).c_str(),
            map_table->name, (int)map_reversed);

        // Based on index map and rules from selected channel mapping table,
        // fill coefficients in the mapping matrix.
        build_table_matrix_(*map_table, map_reversed, out_mapping, in_mapping);
        normalize_matrix_();
    } else {
        roc_log(LogDebug,
                "channel mapper matrix:"
                " selected mapping table: in_chans=%s out_chans=%s table=[diagonal]",
                channel_set_to_str(in_chans).c_str(),
                channel_set_to_str(out_chans).c_str());

        // There is no selected channel mapping table, which can happen if no mapping is
        // needed or no matching table is found. In this case we fall back to diagonal
        // mapping matrix, where each channel is mapped just to itself.
        build_diagonal_matrix_(out_mapping, in_mapping);
    }
}

const ChannelMapTable* ChannelMapperMatrix::select_mapping_table_(
    const IndexMap& out_mapping, const IndexMap& in_mapping, bool& map_reversed) {
    // Find first downmixing table that covers both output and input masks.
    // We assume that tables are sorted from "smaller" to "larger" channel masks.
    for (size_t n = 0; n < ROC_ARRAY_SIZE(ChanMapTables); n++) {
        const ChannelMapTable& tbl = ChanMapTables[n];

        // Table covers our pair of masks directly,
        // which means that we're downmixing.
        if (out_mapping.index_set.is_subset(tbl.out_mask)
            && in_mapping.index_set.is_subset(tbl.in_mask)) {
            map_reversed = false;
            return &tbl;
        }

        // Table covers our pair of masks after being reversed,
        // which means that we're upmixing.
        if (in_mapping.index_set.is_subset(tbl.out_mask)
            && out_mapping.index_set.is_subset(tbl.in_mask)) {
            map_reversed = true;
            return &tbl;
        }
    }

    return NULL;
}

void ChannelMapperMatrix::build_index_mapping_(IndexMap& mapping,
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
            mapping.index_set.toggle_channel(ch, true);
            mapping.index_map[ch] = ch_index++;
        }
    }
}

void ChannelMapperMatrix::build_table_matrix_(const ChannelMapTable& map_table,
                                              bool map_reversed,
                                              const IndexMap& out_mapping,
                                              const IndexMap& in_mapping) {
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

        set_coeff_(out_ch, in_ch, coeff, out_mapping, in_mapping);
    }
}

void ChannelMapperMatrix::build_diagonal_matrix_(const IndexMap& out_mapping,
                                                 const IndexMap& in_mapping) {
    for (size_t ch = 0; ch < ChanPos_Max; ch++) {
        set_coeff_(ch, ch, 1.f, out_mapping, in_mapping);
    }
}

void ChannelMapperMatrix::normalize_matrix_() {
    for (size_t out_ch = 0; out_ch < ChanPos_Max; out_ch++) {
        sample_t coeff_sum = 0;

        for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
            coeff_sum += matrix_[out_ch][in_ch];
        }

        if (coeff_sum == 0.f) {
            continue;
        }

        for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
            matrix_[out_ch][in_ch] /= coeff_sum;
        }
    }
}

void ChannelMapperMatrix::set_coeff_(size_t out_ch,
                                     size_t in_ch,
                                     sample_t value,
                                     const IndexMap& out_mapping,
                                     const IndexMap& in_mapping) {
    if (!out_mapping.index_set.has_channel(out_ch)) {
        return;
    }

    if (!in_mapping.index_set.has_channel(in_ch)) {
        return;
    }

    const size_t out_index = out_mapping.index_map[out_ch];
    const size_t in_index = in_mapping.index_map[in_ch];

    roc_panic_if_not(out_ch < ChanPos_Max && in_ch < ChanPos_Max);
    roc_panic_if_not(out_index < ChanPos_Max && in_index < ChanPos_Max);

    matrix_[out_index][in_index] = value;
}

} // namespace audio
} // namespace roc
