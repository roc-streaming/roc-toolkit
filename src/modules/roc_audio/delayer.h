/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/delayer.h
//! @brief Audio packet delayer.

#ifndef ROC_AUDIO_DELAYER_H_
#define ROC_AUDIO_DELAYER_H_

#include "roc_core/noncopyable.h"

#include "roc_packet/ipacket_reader.h"
#include "roc_packet/packet_queue.h"

namespace roc {
namespace audio {

//! Audio packet delayer.
//! @remarks
//!  Delays audio packet reader for given amount of samples.
class Delayer : public packet::IPacketReader, public core::NonCopyable<> {
public:
    //! Constructor.
    //!
    //! @b Parameters
    //!  - @p reader is input audio packet reader;
    //!  - @p delay is number of samples to be delayed.
    //!
    //! @remarks
    //!  read() returns NULL until packets with total length of at least
    //!  @p delay samples are available first time. After that, read()
    //!  will always return packets from @p reader.
    Delayer(packet::IPacketReader& reader, packet::timestamp_t delay);

    //! Read next packet.
    virtual packet::IPacketConstPtr read();

private:
    packet::timestamp_t queue_size_() const;

    packet::IPacketReader& reader_;
    packet::PacketQueue queue_;
    packet::timestamp_t delay_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_DELAYER_H_
