/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_mapper.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

ChannelMapper::ChannelMapper(const ChannelSet& in_chans, const ChannelSet& out_chans)
    : in_chans_(in_chans)
    , out_chans_(out_chans)
    , inout_chans_(in_chans)
    , matrix_(in_chans_, out_chans_) {
    inout_chans_.bitwise_or(out_chans);
    setup_map_func_();
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
                out_s += in_samples[in_ch] * matrix_.coeff(out_ch, in_ch);
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

} // namespace audio
} // namespace roc
