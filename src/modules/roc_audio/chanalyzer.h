/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/chanalyzer.h
//! @brief Chanalyzer.

#ifndef ROC_AUDIO_CHANALYZER_H_
#define ROC_AUDIO_CHANALYZER_H_

#include "roc_config/config.h"

#include "roc_core/noncopyable.h"
#include "roc_core/array.h"
#include "roc_core/list.h"

#include "roc_packet/ipacket_reader.h"
#include "roc_audio/iaudio_packet_reader.h"

namespace roc {
namespace audio {

//! Demultiplex single packet stream to per-channel streams.
class Chanalyzer : public IAudioPacketReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input packet queue to be multiplexed;
    //!  - @p channels is bitmask of enabled channels.
    Chanalyzer(packet::IPacketReader& reader,
               packet::channel_mask_t channels = ROC_CONFIG_DEFAULT_CHANNEL_MASK);

    //! Read next audio packet for given channel.
    virtual packet::IAudioPacketConstPtr read(packet::channel_t ch);

private:
    static const size_t MaxChannels = ROC_CONFIG_MAX_CHANNELS;

    bool append_();
    void shift_();

    packet::IPacketReader& packet_reader_;
    packet::channel_mask_t channel_mask_;

    core::List<const packet::IAudioPacket> packets_;

    core::Array<packet::IAudioPacketConstPtr, MaxChannels> head_;
    core::Array<size_t, MaxChannels> shift_pos_;
    packet::channel_mask_t shift_mask_;
    size_t min_shift_pos_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANALYZER_H_
