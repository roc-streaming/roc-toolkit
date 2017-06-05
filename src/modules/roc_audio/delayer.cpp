/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/panic.h"

#include "roc_audio/delayer.h"
#include "roc_packet/ipacket.h"

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
        if (!packet->rtp()) {
            roc_panic("delayer: got unexpected packet w/o RTP header");
        }

        if (!packet->audio()) {
            roc_panic("delayer: got unexpected packet w/o audio payload");
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

    packet::timestamp_t head = queue_.head()->rtp()->timestamp();
    packet::timestamp_t tail = queue_.tail()->rtp()->timestamp()
        + (packet::timestamp_t)queue_.tail()->audio()->num_samples();

    return (packet::timestamp_t)ROC_SUBTRACT(packet::signed_timestamp_t, tail, head);
}

} // namespace audio
} // namespace roc
