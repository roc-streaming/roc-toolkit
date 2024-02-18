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
    roc_log(LogDebug,
            "channel mapper matrix: building mapping:"
            " in_chans=%s out_chans=%s",
            channel_set_to_str(in_chans).c_str(), channel_set_to_str(out_chans).c_str());

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

    if (core::Logger::instance().get_level() >= LogTrace) {
        print_index_matrix_(in_index_map, out_index_map);
    }
}

void ChannelMapperMatrix::build_index_mapping_(IndexMap& result_map,
                                               const ChannelSet& ch_set) {
    result_map.enabled_chans.set_layout(ChanLayout_Surround);

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

    ChannelSet cur_in_chans = in_chans;
    ChannelSet actual_out_chans;

    bool is_first = true;

    for (;;) {
        // Select table that covers next step of conversion from input to output mask.
        //
        // Each iteration we determine whether we want to downmix or upmix.
        // Depending on the masks, it may happen that we first need to upmix and only
        // then to downmix. E.g. to map 7.1.2 to 3.1-3c, we will combine the following
        // mappings into one:
        //    7.1.2 --(upmix)-> 7.1.2-3c --(downmix)-> 5.1-3c --(downmix)-> 3.1-3c
        //
        // It is so because mapping tables don't provide explicit mapping for every
        // possible combination of channel masks, as there would be too many of them.
        const bool is_downmixing = can_downmix_(cur_in_chans, out_chans);

        const ChannelMapTable* map_table = NULL;
        if (is_downmixing) {
            if ((map_table = next_downmix_table_(cur_in_chans, out_chans))) {
                actual_out_chans.set_layout(ChanLayout_Surround);
                actual_out_chans.set_mask(map_table->out_mask);
            }
        } else {
            if ((map_table = next_upmix_table_(cur_in_chans, out_chans))) {
                actual_out_chans.set_layout(ChanLayout_Surround);
                actual_out_chans.set_mask(map_table->in_mask);
            }
        }

        // No suitable mapping found, end loop.
        if (!map_table) {
            break;
        }

        roc_log(LogDebug,
                "channel mapper matrix: pulling mapping table:"
                " table=[%s] dir=%s",
                map_table->name, is_downmixing ? "downmix" : "upmix");

        // Build matrix from found table.
        ChannelMap next_map;
        fill_mapping_from_table_(next_map, *map_table, is_downmixing, cur_in_chans,
                                 actual_out_chans);

        if (core::Logger::instance().get_level() >= LogTrace) {
            print_table_matrix_(next_map);
        }

        if (is_first) {
            result_map = next_map;
        } else {
            // Combine subsequent mappings into one matrix.
            combine_mappings_(result_map, next_map);
        }

        // Proceed to next step.
        // On next iteration, we'll request mapping output of current step
        // (actual_out_chans) to desired output (out_chans).
        is_first = false;
        cur_in_chans = actual_out_chans;
    }

    return !is_first;
}

bool ChannelMapperMatrix::can_downmix_(const ChannelSet& in_chans,
                                       const ChannelSet& out_chans) {
    for (size_t i = 0; i < ROC_ARRAY_SIZE(ChanMapTables); i++) {
        const ChannelMask in_mask = ChanMapTables[i].in_mask;

        if (!in_chans.is_subset(in_mask)) {
            continue;
        }

        // Find first cluster of tables which covers in_chans.
        // Check if there is a table in cluster that covers out_chans.
        for (size_t j = i; j < ROC_ARRAY_SIZE(ChanMapTables); j++) {
            if (ChanMapTables[j].in_mask != in_mask) {
                // No more tables in cluster, give up.
                break;
            }

            const ChannelMask out_mask = ChanMapTables[j].out_mask;

            if (out_chans.is_subset(out_mask)) {
                return true;
            }
        }

        // No more tables, give up.
        break;
    }

    return false;
}

const ChannelMapTable*
ChannelMapperMatrix::next_downmix_table_(const ChannelSet& in_chans,
                                         const ChannelSet& out_chans) {
    for (size_t n = 0; n < ROC_ARRAY_SIZE(ChanMapTables); n++) {
        const ChannelMapTable& tbl = ChanMapTables[n];

        // Find first table that covers downmixing from in_chans to out_chans.
        if (in_chans.is_subset(tbl.in_mask) && out_chans.is_subset(tbl.out_mask)
            && !in_chans.is_subset(tbl.out_mask)) {
            return &tbl;
        }
    }

    return NULL;
}

const ChannelMapTable*
ChannelMapperMatrix::next_upmix_table_(const ChannelSet& in_chans,
                                       const ChannelSet& out_chans) {
    const ChannelMapTable* next_tbl = NULL;
    ChannelSet next_chans = out_chans;

    // Channel mapping tables provide only downmixing matrices, and upmixing
    // matrices are automatically inferred from them.
    //
    // To upmix "A -> B", we need to find downmixing table for "B -> A", and revert it.
    // To upmix "A -> B -> C", we need to find downmixing tables "C -> B -> A",
    // revert them, and combine into one. We should do it in reverse order, i.e.
    // first pull reverted "B -> A" table, and then reverted "C -> B" table.
    //
    // Example flow when we want to upmix "A -> B -> C":
    //
    //  1. First call to next_upmix_table_() asks for a table for "A -> B -> C".
    //     We traverse from C to A: "C -> B -> A", and return "B -> A" downmixing table.
    //     Caller reverts it to "A -> B" upmixing table.
    //
    //  2. Second call asks for a table for "B -> C".
    //     We traverse from C to B, and return "C -> B" downmixing table.
    //     Caller reverts it to "B -> C" and combined with the previous one.
    for (;;) {
        const ChannelMapTable* best_tbl = NULL;

        for (ssize_t n = ROC_ARRAY_SIZE(ChanMapTables) - 1; n >= 0; n--) {
            const ChannelMapTable& tbl = ChanMapTables[n];

            // Find last table that covers downmixing from out_chans to in_chans.
            // (We will revert it to upmix from in_chans to out_chans).
            if (in_chans.is_subset(tbl.out_mask) && next_chans.is_subset(tbl.in_mask)
                && !next_chans.is_subset(tbl.out_mask)) {
                best_tbl = &tbl;
            }
        }

        if (!best_tbl) {
            break;
        }

        // Continue backwards search.
        // Output mask of current iteration becomes input of the next one.
        // We repeat the process until we find the last step of downmixing sequence,
        // which will be the first step of upmixing sequence.
        next_tbl = best_tbl;
        next_chans.set_mask(best_tbl->out_mask);
    }

    return next_tbl;
}

void ChannelMapperMatrix::fill_mapping_from_table_(ChannelMap& result_map,
                                                   const ChannelMapTable& map_table,
                                                   bool is_downmixing,
                                                   const ChannelSet& in_chans,
                                                   const ChannelSet& out_chans) {
    // Build matrix based on rules from table.
    ChannelMap temp_map;

    for (size_t n = 0; n < ROC_ARRAY_SIZE(map_table.rules); n++) {
        const ChannelMapRule& rule = map_table.rules[n];
        if (rule.coeff == 0.f) {
            break; // Last rule in table.
        }

        temp_map.chan_matrix[rule.out_ch][rule.in_ch] = rule.coeff;
    }

    // Normalize table matrix.
    normalize_mapping_(temp_map);

    // Get only channels present in masks.
    for (size_t out_ch = 0; out_ch < ChanPos_Max; out_ch++) {
        for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
            if (!out_chans.has_channel(out_ch) || !in_chans.has_channel(in_ch)) {
                continue;
            }

            result_map.chan_matrix[out_ch][in_ch] = is_downmixing
                ? temp_map.chan_matrix[out_ch][in_ch]
                : temp_map.chan_matrix[in_ch][out_ch];
        }
    }

    // Normalize again.
    normalize_mapping_(result_map);
}

void ChannelMapperMatrix::fill_fallback_mapping_(ChannelMap& result_map,
                                                 const ChannelSet& in_chans,
                                                 const ChannelSet& out_chans) {
    roc_log(LogDebug,
            "channel mapper matrix:"
            " selected mapping table: table=[diagonal]");

    // There is no selected channel mapping table, which can happen if no mapping is
    // needed or no matching table is found. In this case we fall back to diagonal
    // mapping matrix, where each channel is mapped just to itself.
    for (size_t ch = 0; ch < ChanPos_Max; ch++) {
        result_map.chan_matrix[ch][ch] = 1.0;
    }
}

void ChannelMapperMatrix::combine_mappings_(ChannelMap& prev_map,
                                            const ChannelMap& next_map) {
    ChannelMap comb_map;

    for (size_t out_ch = 0; out_ch < ChanPos_Max; out_ch++) {
        for (size_t next_ch = 0; next_ch < ChanPos_Max; next_ch++) {
            if (next_map.chan_matrix[out_ch][next_ch] == 0.f) {
                continue;
            }

            for (size_t prev_ch = 0; prev_ch < ChanPos_Max; prev_ch++) {
                if (prev_map.chan_matrix[next_ch][prev_ch] == 0.f) {
                    continue;
                }

                comb_map.chan_matrix[out_ch][prev_ch] +=
                    next_map.chan_matrix[out_ch][next_ch]
                    * prev_map.chan_matrix[next_ch][prev_ch];
            }
        }
    }

    normalize_mapping_(comb_map);

    prev_map = comb_map;
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
    // Combine two index mappings (for channel reordering) and one channel mapping (for
    // downmixing/upmixing) into a single matrix that performs all transformations.
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

            index_matrix_[out_index][in_index] =
                (sample_t)chan_map.chan_matrix[out_ch][in_ch];
        }
    }
}

void ChannelMapperMatrix::print_table_matrix_(const ChannelMap& chan_map) {
    // Prints intermediate mapping build from a single downmixing table.
    // (Uses logical channel positions, not physical indices.)
    core::Printer prn;

    prn.writef("@ table mapping [%dx%d]\n", (int)ChanPos_Max, (int)ChanPos_Max);

    prn.writef("     ");

    for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
        prn.writef(" %5s", channel_pos_to_str((ChannelPosition)in_ch));
    }

    prn.writef("\n");

    for (size_t out_ch = 0; out_ch < ChanPos_Max; out_ch++) {
        prn.writef(" %3s ", channel_pos_to_str((ChannelPosition)out_ch));

        for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
            if (chan_map.chan_matrix[out_ch][in_ch] == 0) {
                prn.writef(" .....");
            } else {
                prn.writef(" %.3f", (double)chan_map.chan_matrix[out_ch][in_ch]);
            }
        }

        prn.writef("\n");
    }
}

void ChannelMapperMatrix::print_index_matrix_(const IndexMap& in_index_map,
                                              const IndexMap& out_index_map) {
    // Print resulting matrix that combines channel reordering and mapping.
    // (Uses physical channel indices).
    core::Printer prn;

    prn.writef("@ final mapping [%dx%d]\n",
               (int)out_index_map.enabled_chans.num_channels(),
               (int)in_index_map.enabled_chans.num_channels());

    prn.writef("     ");

    for (size_t in_index = 0; in_index < ChanPos_Max; in_index++) {
        if (in_index_map.index_2_chan[in_index] == ChanPos_Max) {
            break;
        }

        const ChannelPosition in_ch = in_index_map.index_2_chan[in_index];

        prn.writef(" %5s", channel_pos_to_str(in_ch));
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
                prn.writef(" .....");
            } else {
                prn.writef(" %.3f", (double)index_matrix_[out_index][in_index]);
            }
        }

        prn.writef("\n");
    }
}

} // namespace audio
} // namespace roc
