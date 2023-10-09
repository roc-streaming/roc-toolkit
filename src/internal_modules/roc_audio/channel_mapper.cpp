/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper.h"
#include "roc_audio/channel_mapper_table.h"
#include "roc_audio/channel_set_to_str.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ChannelMapper::ChannelMapper(const ChannelSet& in_chans, const ChannelSet& out_chans)
    : in_chans_(in_chans)
    , out_chans_(out_chans) {
    if (!in_chans_.is_valid()) {
        roc_panic("channel mapper: invalid input channel set: %s",
                  channel_set_to_str(in_chans_).c_str());
    }

    if (!out_chans_.is_valid()) {
        roc_panic("channel mapper: invalid output channel set: %s",
                  channel_set_to_str(out_chans_).c_str());
    }

    inout_chans_ = in_chans;
    inout_chans_.bitwise_or(out_chans);

    setup_map_func_();
    setup_map_matrix_();
}

void ChannelMapper::map(const sample_t* in_samples,
                        size_t n_in_samples,
                        sample_t* out_samples,
                        size_t n_out_samples) {
    if (!in_samples) {
        roc_panic("channel mapper: input buffer is null");
    }

    if (!out_samples) {
        roc_panic("channel mapper: output buffer is null");
    }

    if (n_in_samples % in_chans_.num_channels() != 0) {
        roc_panic("channel mapper: invalid input buffer size:"
                  " in_samples=%lu in_chans=%lu",
                  (unsigned long)n_in_samples, (unsigned long)in_chans_.num_channels());
    }

    if (n_out_samples % out_chans_.num_channels() != 0) {
        roc_panic("channel mapper: invalid output buffer size:"
                  " out_samples=%lu out_chans=%lu",
                  (unsigned long)n_out_samples, (unsigned long)out_chans_.num_channels());
    }

    if (n_in_samples / in_chans_.num_channels()
        != n_out_samples / out_chans_.num_channels()) {
        roc_panic("channel mapper: mismatching buffer sizes:"
                  " in_samples=%lu out_samples=%lu",
                  (unsigned long)n_in_samples, (unsigned long)n_out_samples);
    }

    const size_t n_samples_per_chan = n_in_samples / in_chans_.num_channels();

    (this->*map_func_)(in_samples, out_samples, n_samples_per_chan);
}

// Map between two surround channel sets.
// Each output channel is a sum of input channels multiplied by coefficients
// from the mapping matrix.
void ChannelMapper::map_surround_surround_(const sample_t* in_samples,
                                           sample_t* out_samples,
                                           size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        for (size_t out_ch = 0; out_ch < out_chans_.num_channels(); out_ch++) {
            sample_t out_s = 0;

            for (size_t in_ch = 0; in_ch < in_chans_.num_channels(); in_ch++) {
                out_s += in_samples[in_ch] * map_matrix_[out_ch][in_ch];
            }

            out_s = std::min(out_s, SampleMax);
            out_s = std::max(out_s, SampleMin);

            *out_samples++ = out_s;
        }

        in_samples += in_chans_.num_channels();
    }
}

// Map between surround and multitrack channel sets.
// Copies first N channels of input to first N channels of output,
// ignoring meaning of the channels.
void ChannelMapper::map_multitrack_surround_(const sample_t* in_samples,
                                             sample_t* out_samples,
                                             size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        size_t out_ch = 0;
        size_t in_ch = 0;

        while (out_ch < out_chans_.num_channels()) {
            if (in_ch < in_chans_.num_channels()) {
                *out_samples = *in_samples;
                in_samples++;
                in_ch++;
            } else {
                *out_samples = 0;
            }

            out_samples++;
            out_ch++;
        }

        if (in_ch < in_chans_.num_channels()) {
            in_samples += (in_chans_.num_channels() - in_ch);
        }
    }
}

// Map between two multitrack channel sets.
// Copies tracks that are present in both output and input, zeroises
// tracks that are present in output and are missing in input.
void ChannelMapper::map_multitrack_multitrack_(const sample_t* in_samples,
                                               sample_t* out_samples,
                                               size_t n_samples) {
    for (size_t ns = 0; ns < n_samples; ns++) {
        for (size_t ch = inout_chans_.first_channel(); ch <= inout_chans_.last_channel();
             ch++) {
            if (in_chans_.has_channel(ch)) {
                if (out_chans_.has_channel(ch)) {
                    *out_samples++ = *in_samples;
                }
                in_samples++;
            } else {
                if (out_chans_.has_channel(ch)) {
                    *out_samples++ = 0;
                }
            }
        }
    }
}

void ChannelMapper::setup_map_func_() {
    switch (in_chans_.layout()) {
    case ChanLayout_None:
        break;

    case ChanLayout_Surround:
        switch (out_chans_.layout()) {
        case ChanLayout_None:
            break;

        case ChanLayout_Surround:
            map_func_ = &ChannelMapper::map_surround_surround_;
            break;

        case ChanLayout_Multitrack:
            map_func_ = &ChannelMapper::map_multitrack_surround_;
            break;
        }
        break;

    case ChanLayout_Multitrack:
        switch (out_chans_.layout()) {
        case ChanLayout_None:
            break;

        case ChanLayout_Surround:
            map_func_ = &ChannelMapper::map_multitrack_surround_;
            break;

        case ChanLayout_Multitrack:
            map_func_ = &ChannelMapper::map_multitrack_multitrack_;
            break;
        }
        break;
    }

    if (!map_func_) {
        roc_panic("channel mapper: can't select mapper function");
    }
}

void ChannelMapper::setup_map_matrix_() {
    // Set all coefficients to zero.
    memset(map_matrix_, 0, sizeof(map_matrix_));

    // Mapping matrix is used only when mapping surround to surround.
    if (in_chans_.layout() != ChanLayout_Surround
        || out_chans_.layout() != ChanLayout_Surround) {
        return;
    }

    // Surround layouts should have only channels defined by ChannelPosition.
    roc_panic_if_not(out_chans_.last_channel() < ChanPos_Max);
    roc_panic_if_not(in_chans_.last_channel() < ChanPos_Max);

    // Surround layouts should have valid order.
    roc_panic_if_not(out_chans_.order() > ChanOrder_None
                     && out_chans_.order() < ChanOrder_Max);
    roc_panic_if_not(out_chans_.order() > ChanOrder_None
                     && in_chans_.order() < ChanOrder_Max);

    // Fill mapping of output channel position to its index in frame.
    ChannelSet out_index_set;
    size_t out_index_map[ChanPos_Max] = {};

    const ChannelList& out_order = chan_orders[out_chans_.order()];

    for (size_t out_index = 0, n_ord = 0; out_order.chans[n_ord] != ChanPos_Max;
         n_ord++) {
        const ChannelPosition out_ch = out_order.chans[n_ord];
        if (out_chans_.has_channel(out_ch)) {
            out_index_set.set_channel(out_ch, true);
            out_index_map[out_ch] = out_index++;
        }
    }

    // Fill mapping of input channel position to its index in frame.
    ChannelSet in_index_set;
    size_t in_index_map[ChanPos_Max] = {};

    const ChannelList& in_order = chan_orders[in_chans_.order()];

    for (size_t in_index = 0, n_ord = 0; in_order.chans[n_ord] != ChanPos_Max; n_ord++) {
        const ChannelPosition in_ch = in_order.chans[n_ord];
        if (in_chans_.has_channel(in_ch)) {
            in_index_set.set_channel(in_ch, true);
            in_index_map[in_ch] = in_index++;
        }
    }

    // Find channel map that covers requested transformation.
    const ChannelMap* ch_map = NULL;
    bool is_reverse = false;

    if (out_index_set != in_index_set) {
        for (size_t n = 0; !ch_map && n < ROC_ARRAY_SIZE(chan_maps); n++) {
            if (out_index_set.is_subset(chan_maps[n].out_mask)
                && in_index_set.is_subset(chan_maps[n].in_mask)) {
                ch_map = &chan_maps[n];
                is_reverse = false;
            } else if (in_index_set.is_subset(chan_maps[n].out_mask)
                       && out_index_set.is_subset(chan_maps[n].in_mask)) {
                // This channel map describes reversed transformation.
                ch_map = &chan_maps[n];
                is_reverse = true;
            }
        }
    }

    if (ch_map) {
        // Fill mapping matrix from rules in found channel map.
        roc_log(LogDebug,
                "channel mapper:"
                " selected mapping: in_chans=%s out_chans=%s map=[%s] is_reverse=%d",
                channel_set_to_str(in_chans_).c_str(),
                channel_set_to_str(out_chans_).c_str(), ch_map->name, (int)is_reverse);

        for (size_t n = 0; n < ROC_ARRAY_SIZE(ch_map->rules); n++) {
            const ChannelMapRule& rule = ch_map->rules[n];
            if (rule.coeff == 0.f) {
                // Last rule.
                break;
            }

            ChannelPosition out_ch, in_ch;
            sample_t coeff;

            if (!is_reverse) {
                out_ch = rule.out_ch;
                in_ch = rule.in_ch;
                coeff = rule.coeff;
            } else {
                out_ch = rule.in_ch;
                in_ch = rule.out_ch;
                coeff = 1.f / rule.coeff;
            }

            roc_panic_if_not(out_ch < ChanPos_Max && in_ch < ChanPos_Max);
            roc_panic_if_not(out_index_map[out_ch] < ChanPos_Max
                             && in_index_map[in_ch] < ChanPos_Max);

            if (out_index_set.has_channel(out_ch) && in_index_set.has_channel(in_ch)) {
                map_matrix_[out_index_map[out_ch]][in_index_map[in_ch]] = coeff;
            }
        }

        // Normalize mapping matrix.
        for (size_t out_ch = 0; out_ch < ChanPos_Max; out_ch++) {
            sample_t coeff_sum = 0;

            for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
                coeff_sum += map_matrix_[out_ch][in_ch];
            }

            if (coeff_sum != 0.f) {
                for (size_t in_ch = 0; in_ch < ChanPos_Max; in_ch++) {
                    map_matrix_[out_ch][in_ch] /= coeff_sum;
                }
            }
        }
    } else {
        // No mapping found.
        // Use fallback mapping matrix, where each channel is mapped only to itself.
        roc_log(LogDebug,
                "channel mapper:"
                " selected mapping: in_chans=%s out_chans=%s map=[diagonal]",
                channel_set_to_str(in_chans_).c_str(),
                channel_set_to_str(out_chans_).c_str());

        for (size_t ch = 0; ch < ChanPos_Max; ch++) {
            roc_panic_if_not(out_index_map[ch] < ChanPos_Max
                             && in_index_map[ch] < ChanPos_Max);

            if (out_index_set.has_channel(ch) && in_index_set.has_channel(ch)) {
                map_matrix_[out_index_map[ch]][in_index_map[ch]] = 1.f;
            }
        }
    }
}

} // namespace audio
} // namespace roc
