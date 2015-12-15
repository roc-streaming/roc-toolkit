/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/channel_muxer.h"

namespace roc {
namespace audio {

ChannelMuxer::ChannelMuxer(packet::channel_mask_t channels,
                           ISampleBufferComposer& composer)
    : zipper_(composer)
    , channels_(channels) {
    if (channels_ == 0) {
        roc_panic("channel muxer: channel mask is zero");
    }

    for (size_t ch = 0; ch < mixers_.max_size(); ch++) {
        new (mixers_.allocate()) Mixer(composer);

        if (channels_ & (1 << ch)) {
            zipper_.add(mixers_.back());
        }
    }
}

void ChannelMuxer::attach(packet::channel_t ch, IStreamReader& reader) {
    if ((channels_ & (1 << ch)) == 0) {
        roc_panic("channel muxer: can't attach reader for disabled channel %d", (int)ch);
    }

    mixers_[ch].add(reader);
}

void ChannelMuxer::detach(packet::channel_t ch, IStreamReader& reader) {
    if ((channels_ & (1 << ch)) == 0) {
        roc_panic("channel muxer: can't detach reader for disabled channel %d", (int)ch);
    }

    mixers_[ch].remove(reader);
}

void ChannelMuxer::read(const ISampleBufferSlice& buffer) {
    zipper_.read(buffer);
}

} // namespace audio
} // namespace roc
