/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/link_meter.h"
#include "roc_core/panic.h"
#include "roc_packet/units.h"

namespace roc {
namespace rtp {

LinkMeter::LinkMeter(const EncodingMap& encoding_map)
    : encoding_map_(encoding_map)
    , encoding_(NULL)
    , writer_(NULL)
    , reader_(NULL)
    , first_packet_(true)
    , has_metrics_(false)
    , first_seqnum_(0)
    , last_seqnum_hi_(0)
    , last_seqnum_lo_(0) {
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
        // Since we don't know packet type in-before, we also determine
        // encoding dynamically.
        if (!encoding_ || encoding_->payload_type != packet->rtp()->payload_type) {
            encoding_ = encoding_map_.find_by_pt(packet->rtp()->payload_type);
        }
        if (encoding_) {
            update_metrics_(*packet);
        }
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
    const packet::seqnum_t pkt_seqnum = packet.rtp()->seqnum;

    // If packet seqnum is before first seqnum, and there was no wrap yet,
    // update first seqnum.
    if ((first_packet_ || packet::seqnum_diff(pkt_seqnum, first_seqnum_) < 0)
        && last_seqnum_hi_ == 0) {
        first_seqnum_ = pkt_seqnum;
    }

    // If packet seqnum is after last seqnum, update last seqnum, and
    // also counts possible wraps.
    if (first_packet_ || packet::seqnum_diff(pkt_seqnum, last_seqnum_lo_) > 0) {
        if (pkt_seqnum < last_seqnum_lo_) {
            last_seqnum_hi_ += (uint16_t)-1;
        }
        last_seqnum_lo_ = pkt_seqnum;
    }

    metrics_.sample_rate = encoding_->sample_spec.sample_rate();

    metrics_.ext_first_seqnum = first_seqnum_;
    metrics_.ext_last_seqnum = last_seqnum_hi_ + last_seqnum_lo_;

    first_packet_ = false;
    has_metrics_ = true;
}

} // namespace rtp
} // namespace roc
