/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iaudio_packet_reader.h
//! @brief Audio packet reader interface.

#ifndef ROC_AUDIO_IAUDIO_PACKET_READER_H_
#define ROC_AUDIO_IAUDIO_PACKET_READER_H_

#include "roc_packet/iaudio_packet.h"

namespace roc {
namespace audio {

//! Audio packet reader interface.
class IAudioPacketReader {
public:
    virtual ~IAudioPacketReader();

    //! Read next audio packet for given channel.
    //! @returns
    //!  next available packet for channel @p ch or NULL if there are no packets.
    //! @see
    //!  Also see notes for packet::IPacketReader.
    virtual packet::IAudioPacketConstPtr read(packet::channel_t ch) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IAUDIO_PACKET_READER_H_
