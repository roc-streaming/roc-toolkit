/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/sequencer.h"
#include "roc_core/fast_random.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtp {

Sequencer::Sequencer(Identity& identity, unsigned int payload_type)
    : identity_(identity)
    , payload_type_(payload_type)
    , seqnum_(0)
    , stream_ts_(0)
    , init_status_(status::NoStatus) {
    // Start with random RTP seqnum and timestamp, as required by RFC 3550.
    seqnum_ = (packet::seqnum_t)core::fast_random_range(0, packet::seqnum_t(-1));
    stream_ts_ = (packet::stream_timestamp_t)core::fast_random_range(
        0, packet::stream_timestamp_t(-1));

    init_status_ = status::StatusOK;
}

status::StatusCode Sequencer::init_status() const {
    return init_status_;
}

void Sequencer::next(packet::Packet& packet,
                     core::nanoseconds_t capture_ts,
                     packet::stream_timestamp_t duration) {
    roc_panic_if(init_status_ != status::StatusOK);

    packet::RTP* rtp = packet.rtp();
    if (!rtp) {
        roc_panic("rtp sequencer: unexpected non-rtp packet");
    }

    // Identity can change SSRC in case of collision, so we read SSRC
    // from it each time.
    rtp->source_id = identity_.ssrc();
    rtp->payload_type = payload_type_;
    rtp->seqnum = seqnum_;
    rtp->stream_timestamp = stream_ts_;
    rtp->duration = duration;
    rtp->capture_timestamp = capture_ts;

    seqnum_++;
    stream_ts_ += duration;
}

} // namespace rtp
} // namespace roc
