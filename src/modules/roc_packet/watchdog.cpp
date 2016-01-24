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
#include "roc_core/math.h"

#include "roc_packet/watchdog.h"

#define SEQ_IS_BEFORE(a, b) ROC_IS_BEFORE(signed_seqnum_t, a, b)
#define SEQ_SUBTRACT(a, b) ROC_SUBTRACT(signed_seqnum_t, a, b)
#define TS_SUBTRACT(a, b) ROC_SUBTRACT(signed_timestamp_t, a, b)

namespace roc {
namespace packet {

Watchdog::Watchdog(IPacketReader& reader, size_t timeout, size_t rate)
    : reader_(reader)
    , rate_(rate)
    , timeout_(timeout)
    , countdown_(timeout)
    , has_packets_(false)
    , alive_(true) {
}

bool Watchdog::update() {
    if (!alive_) {
        return false;
    }

    if (has_packets_) {
        countdown_ = timeout_;
    } else {
        if (countdown_ > 0) {
            countdown_--;
        }
        if (countdown_ == 0) {
            roc_log(LogInfo, "watchdog: timeout reached (%u ticks without packets)",
                    (unsigned)timeout_);
            return (alive_ = false);
        }
    }

    has_packets_ = false;
    return true;
}

IPacketConstPtr Watchdog::read() {
    if (!alive_) {
        return NULL;
    }

    IPacketConstPtr packet = reader_.read();
    if (!packet) {
        return NULL;
    }

    if (!packet->rtp()) {
        roc_log(LogInfo, "watchdog: unexpected non-RTP packet");
        return NULL;
    }

    if (packet->rtp()->rate() != rate_) {
        roc_log(LogInfo, "watchdog: unexpected rate: got=%u expected=%u",
                (unsigned)packet->rtp()->rate(), (unsigned)rate_);
        return NULL;
    }

    if (detect_jump_(packet)) {
        alive_ = false;
        return NULL;
    }

    has_packets_ = true;

    return packet;
}

bool Watchdog::detect_jump_(const IPacketConstPtr& next) {
    const IHeaderRTP* nh = next->rtp();
    const IHeaderRTP* ph = NULL;

    if (prev_) {
        ph = prev_->rtp();
        if (ph->source() != nh->source()) {
            roc_log(LogInfo, "watchdog: source id jump: prev=%lu next=%lu",
                    (unsigned long)ph->source(), (unsigned long)nh->source());
            return true;
        }

        signed_seqnum_t sn_dist = SEQ_SUBTRACT(nh->seqnum(), ph->seqnum());

        if (ROC_ABS(sn_dist) > ROC_CONFIG_MAX_SN_JUMP) {
            roc_log(LogInfo, "watchdog: too long seqnum jump:"
                             " prev=%lu next=%lu dist=%ld",
                    (unsigned long)ph->seqnum(), (unsigned long)nh->seqnum(),
                    (long)sn_dist);
            return true;
        }

        signed_timestamp_t ts_dist = TS_SUBTRACT(nh->timestamp(), ph->timestamp());

        if (ROC_ABS(ts_dist) > ROC_CONFIG_MAX_TS_JUMP) {
            roc_log(LogInfo, "watchdog: too long timestamp jump:"
                             " prev=%lu next=%lu dist=%ld",
                    (unsigned long)ph->timestamp(), (unsigned long)nh->timestamp(),
                    (long)ts_dist);
            return true;
        }
    }

    if (!prev_ || SEQ_IS_BEFORE(ph->seqnum(), nh->seqnum())) {
        prev_ = next;
    }

    return false;
}

} // namespace packet
} // namespace roc
