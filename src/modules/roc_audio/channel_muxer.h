/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_muxer.h
//! @brief Channel muxer.

#ifndef ROC_AUDIO_CHANNEL_MUXER_H_
#define ROC_AUDIO_CHANNEL_MUXER_H_

#include "roc_config/config.h"

#include "roc_core/array.h"
#include "roc_core/noncopyable.h"

#include "roc_audio/isink.h"
#include "roc_audio/istream_reader.h"
#include "roc_audio/mixer.h"
#include "roc_audio/sample_buffer.h"
#include "roc_audio/zipper.h"

namespace roc {
namespace audio {

//! Channel muxer.
//!
//! Merges multiple streams on multiple channels into single
//! interleaved stream.
class ChannelMuxer : public IStreamReader, public ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    explicit ChannelMuxer(
        packet::channel_mask_t channels = ROC_CONFIG_DEFAULT_CHANNEL_MASK,
        ISampleBufferComposer& composer = default_buffer_composer());

    //! Attach reader for channel.
    virtual void attach(packet::channel_t, IStreamReader&);

    //! Detach reader for channel.
    virtual void detach(packet::channel_t, IStreamReader&);

    //! Read combined audio stream.
    virtual void read(const ISampleBufferSlice&);

private:
    static const size_t MaxChannels = ROC_CONFIG_MAX_CHANNELS;

    core::Array<Mixer, MaxChannels> mixers_;
    Zipper zipper_;

    const packet::channel_mask_t channels_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MUXER_H_
