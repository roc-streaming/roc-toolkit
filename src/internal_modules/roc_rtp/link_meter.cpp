/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/link_meter.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtp {

LinkMeter::LinkMeter()
    : writer_(NULL)
    , reader_(NULL)
    , first_packet_(true)
    , has_metrics_(false)
    , seqnum_hi_(0)
    , seqnum_lo_(0) {
}

status::StatusCode LinkMeter::write(const packet::PacketPtr& packet) {
    if (!writer_) {
        roc_panic("link meter: forgot to call set_writer()");
    }

    if (!packet) {
        roc_panic("link meter: null packet");
    }

    // When we create LinkMeter, we don't know yet if RTP is used (e.g.
    // for repair packets), so we should be ready for non-rtp packets.
    if (packet->rtp()) {
        update_metrics_(*packet);
    }

    return writer_->write(packet);
}

status::StatusCode LinkMeter::read(packet::PacketPtr& packet) {
    if (!reader_) {
        roc_panic("link meter: forgot to call set_reader()");
    }

    const status::StatusCode code = reader_->read(packet);
    if (code != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

void LinkMeter::set_writer(packet::IWriter& writer) {
    writer_ = &writer;
}

void LinkMeter::set_reader(packet::IReader& reader) {
    reader_ = &reader;
}

bool LinkMeter::has_metrics() const {
    return has_metrics_;
}

LinkMetrics LinkMeter::metrics() const {
    return metrics_;
}

void LinkMeter::update_metrics_(const packet::Packet& packet) {
    // Check if packet's seqnum goes ahead of the previous seqnum,
    // taken possible wrap into account.
    if (first_packet_ || packet::seqnum_diff(packet.rtp()->seqnum, seqnum_lo_) > 0) {
        if (packet.rtp()->seqnum < seqnum_lo_) {
            // Detect wrap.
            seqnum_hi_ += (uint16_t)-1;
        }
        seqnum_lo_ = packet.rtp()->seqnum;
        metrics_.ext_last_seqnum = seqnum_hi_ + seqnum_lo_;
    }

    first_packet_ = false;
    has_metrics_ = true;
}

} // namespace rtp
} // namespace roc
