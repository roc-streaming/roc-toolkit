/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper_matrix.h"
#include "roc_audio/channel_mapper_table.h"
#include "roc_audio/channel_set_to_str.h"

namespace roc {
namespace audio {

ChannelMapperMatrix::ChannelMapperMatrix(const ChannelSet& in_chans,
                                         const ChannelSet& out_chans) {
    memset(matrix_, 0, sizeof(matrix_));

    if (!in_chans.is_valid()) {
        roc_panic("channel mapper matrix: invalid input channel set: %s",
                  channel_set_to_str(in_chans).c_str());
    }

    if (!out_chans.is_valid()) {
        roc_panic("channel mapper matrix: invalid output channel set: %s",
                  channel_set_to_str(out_chans).c_str());
    }

    if (in_chans.layout() != ChanLayout_Surround
        || out_chans.layout() != ChanLayout_Surround) {
        return;
    }

    // Surround layouts should have only channels defined by ChannelPosition.
    roc_panic_if_not(out_chans.last_channel() < ChanPos_Max);
    roc_panic_if_not(in_chans.last_channel() < ChanPos_Max);

    // Surround layouts should have valid order.
    roc_panic_if_not(out_chans.order() > ChanOrder_None
                     && out_chans.order() < ChanOrder_Max);
    roc_panic_if_not(out_chans.order() > ChanOrder_None
                     && in_chans.order() < ChanOrder_Max);

    Mapping out_mapping(out_chans);
    Mapping in_mapping(in_chans);

    bool is_reverse = false;
    const ChannelMap* ch_map = find_channel_map_(out_mapping, in_mapping, is_reverse);
    if (ch_map) {
        roc_log(
            LogDebug,
            "channel mapper matrix:"
            " selected mapping table: in_chans=%s out_chans=%s table=[%s] is_reverse=%d",
            channel_set_to_str(in_chans).c_str(), channel_set_to_str(out_chans).c_str(),
            ch_map->name, (int)is_reverse);

        set_map_(*ch_map, is_reverse, out_mapping, in_mapping);
        normalize_();
    } else {
        roc_log(LogDebug,
                "channel mapper matrix:"
                " selected mapping table: in_chans=%s out_chans=%s table=[diagonal]",
                channel_set_to_str(in_chans).c_str(),
                channel_set_to_str(out_chans).c_str());

        set_fallback_(out_mapping, in_mapping);
    }
}

sample_t ChannelMapperMatrix::coeff(size_t out_ch, size_t in_ch) const {
    return matrix_[out_ch][in_ch];
}

const ChannelMap* ChannelMapperMatrix::find_channel_map_(const Mapping& out_mapping,
                                                         const Mapping& in_mapping,
                                                         bool& is_reverse) {
    if (out_mapping.index_set == in_mapping.index_set) {
        return NULL;
    }

    for (size_t n = 0; n < ROC_ARRAY_SIZE(chan_maps); n++) {
        // Downmixing.
        if (out_mapping.index_set.is_subset(chan_maps[n].out_mask)
            && in_mapping.index_set.is_subset(chan_maps[n].in_mask)) {
            is_reverse = false;
            return &chan_maps[n];
        }

        // Upmixing.
        if (in_mapping.index_set.is_subset(chan_maps[n].out_mask)
            && out_mapping.index_set.is_subset(chan_maps[n].in_mask)) {
            is_reverse = true;
            return &chan_maps[n];
        }
    }

    return NULL;
}

ChannelMapperMatrix::Mapping::Mapping(const ChannelSet& chs) {
    memset(index_map, 0, sizeof(index_map));

    const ChannelList& order = chan_orders[chs.order()];

    size_t off = 0;
    size_t pos = 0;

    while (true) {
        const ChannelPosition ch = order.chans[pos];
        if (ch == ChanPos_Max) {
            break;
        }

        if (chs.has_channel(ch)) {
            index_set.set_channel(ch, true);
            index_map[ch] = off++;
        }

        ++pos;
    }
}

void ChannelMapperMatrix::set_fallback_(const Mapping& out_mapping,
                                        const Mapping& in_mapping) {
    for (size_t n = 0; n < ChanPos_Max; n++) {
        set_(n, n, 1.f, out_mapping, in_mapping);
    }
}

void ChannelMapperMatrix::set_map_(const ChannelMap& map,
                                   bool is_reverse,
                                   const Mapping& out_mapping,
                                   const Mapping& in_mapping) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(map.rules); n++) {
        const ChannelMapRule& rule = map.rules[n];
        if (rule.coeff == 0.f) {
            // Last rule.
            break;
        }

        ChannelPosition out_ch = ChanPos_Max;
        ChannelPosition in_ch = ChanPos_Max;
        sample_t coeff = 0.f;

        if (!is_reverse) {
            out_ch = rule.out_ch;
            in_ch = rule.in_ch;
            coeff = rule.coeff;
        } else {
            out_ch = rule.in_ch;
            in_ch = rule.out_ch;
            coeff = 1.f / rule.coeff;
        }

        set_(out_ch, in_ch, coeff, out_mapping, in_mapping);
    }
}

void ChannelMapperMatrix::normalize_() {
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

void ChannelMapperMatrix::set_(size_t out_ch,
                               size_t in_ch,
                               sample_t value,
                               const Mapping& out_mapping,
                               const Mapping& in_mapping) {
    const size_t out_off = out_mapping.index_map[out_ch];
    const size_t in_off = in_mapping.index_map[in_ch];

    roc_panic_if_not(out_ch < ChanPos_Max);
    roc_panic_if_not(in_ch < ChanPos_Max);
    roc_panic_if_not(out_off < ChanPos_Max);
    roc_panic_if_not(in_off < ChanPos_Max);

    if (!out_mapping.index_set.has_channel(out_ch)) {
        return;
    }
    if (!in_mapping.index_set.has_channel(in_ch)) {
        return;
    }

    matrix_[out_off][in_off] = value;
}

} // namespace audio
} // namespace roc
