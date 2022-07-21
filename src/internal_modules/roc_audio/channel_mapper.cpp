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

ChannelMapper::ChannelMapper(packet::channel_mask_t in_chans,
                             packet::channel_mask_t out_chans)
    : in_chan_mask_(in_chans)
    , out_chan_mask_(out_chans)
    , in_chan_count_(packet::num_channels(in_chans))
    , out_chan_count_(packet::num_channels(out_chans))
    , inout_chan_mask_(in_chans | out_chans) {
}

void ChannelMapper::map(const Frame& in_frame, Frame& out_frame) {
    if (in_frame.num_samples() % in_chan_count_ != 0) {
        roc_panic("channel mapper: unexpected input frame size");
    }

    if (out_frame.num_samples() % out_chan_count_ != 0) {
        roc_panic("channel mapper: unexpected output frame size");
    }

    if (in_frame.num_samples() / in_chan_count_ != out_frame.num_samples() / out_chan_count_) {
        roc_panic("channel mapper: mismatching frame sizes");
    }

    const size_t n_samples = in_frame.num_samples() / in_chan_count_;

    const sample_t* in_samples = in_frame.samples();
    sample_t* out_samples = out_frame.samples();

    for (size_t ns = 0; ns < n_samples; ns++) {
        for (packet::channel_mask_t ch = 1; ch <= inout_chan_mask_ && ch != 0; ch <<= 1) {
            if (in_chan_mask_ & ch) {
                if (out_chan_mask_ & ch) {
                    *out_samples++ = *in_samples;
                }
                in_samples++;
            } else {
                if (out_chan_mask_ & ch) {
                    *out_samples++ = 0;
                }
            }
        }
    }
}

} // namespace audio
} // namespace roc
