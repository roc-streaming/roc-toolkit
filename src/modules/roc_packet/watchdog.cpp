/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/watchdog.h"
#include "roc_core/log.h"

namespace roc {
namespace packet {

Watchdog::Watchdog(IReader& reader, timestamp_t timeout)
    : reader_(reader)
    , timeout_(timeout)
    , update_time_(0)
    , read_time_(0)
    , first_(true)
    , alive_(true) {
}

PacketPtr Watchdog::read() {
    if (!alive_) {
        return NULL;
    }

    PacketPtr packet = reader_.read();
    if (!packet) {
        return NULL;
    }

    read_time_ = update_time_;

    return packet;
}

bool Watchdog::update(timestamp_t time) {
    if (!alive_) {
        return false;
    }

    if (first_) {
        read_time_ = time;
        first_ = false;
    }

    update_time_ = time;

    if (update_time_ - read_time_ >= timeout_) {
        roc_log(LogInfo,
                "watchdog: timeout reached: update_time=%lu read_time=%lu timeout=%lu",
                (unsigned long)update_time_, (unsigned long)read_time_,
                (unsigned long)timeout_);
        alive_ = false;
        return false;
    }

    return true;
}

} // namespace packet
} // namespace roc
