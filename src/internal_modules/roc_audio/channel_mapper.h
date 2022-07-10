/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_mapper.h
//! @brief Mixer.

#ifndef ROC_AUDIO_CHANNEL_MAPPER_H_
#define ROC_AUDIO_CHANNEL_MAPPER_H_

#include "roc_audio/frame.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Channel mapper.
//! Converts between frames with specified channel masks.
class ChannelMapper : public core::NonCopyable<> {
public:
    //! Initialize.
    ChannelMapper(packet::channel_mask_t in_chans, packet::channel_mask_t out_chans);

    //! Map frame.
    void map(const Frame& in_frame, Frame& out_frame);

private:
    const packet::channel_mask_t in_chan_mask_;
    const packet::channel_mask_t out_chan_mask_;

    const size_t in_chan_count_;
    const size_t out_chan_count_;

    const packet::channel_mask_t inout_chan_mask_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_H_
