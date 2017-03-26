/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/panic.h"
#include "roc_core/log.h"

#include "roc_packet/iaudio_packet.h"
#include "roc_audio/delayer.h"

namespace roc {
namespace audio {

Delayer::Delayer(packet::IPacketReader& reader, packet::timestamp_t delay)
    : reader_(reader)
    , queue_(0)
    , delay_(delay) {
}

packet::IPacketConstPtr Delayer::read() {
    if (delay_ == 0 && queue_.size() == 0) {
        return reader_.read();
    }

    while (packet::IPacketConstPtr packet = reader_.read()) {
        if (packet->type() != packet::IAudioPacket::Type) {
            roc_panic("delayer: got packet of wrong type (expected audio packet)");
        }
        queue_.write(packet);
    }

    if (delay_ != 0) {
        const packet::timestamp_t qs = queue_size_();
        if (qs <= delay_) {
            return NULL;
        }

        roc_log(LogInfo,
                "delayer: received enough packets: delay=%lu samples=%lu packets=%lu",
                (unsigned long)delay_, //
                (unsigned long)qs,     //
                (unsigned long)queue_.size());

        delay_ = 0;
    }

    return queue_.read();
}

packet::timestamp_t Delayer::queue_size_() const {
    if (queue_.size() == 0) {
        return 0;
    }

    const packet::IAudioPacket* head =
        static_cast<const packet::IAudioPacket*>(queue_.head().get());

    const packet::IAudioPacket* tail =
        static_cast<const packet::IAudioPacket*>(queue_.tail().get());

    return (packet::timestamp_t)ROC_SUBTRACT(packet::signed_timestamp_t,
                                             tail->timestamp() + tail->num_samples(),
                                             head->timestamp());
}

} // namespace audio
} // namespace roc
