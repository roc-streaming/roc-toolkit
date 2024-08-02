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

bool LinkMeterConfig::deduce_defaults(audio::LatencyTunerProfile latency_profile) {
    if (sliding_window_length == 0) {
        if (latency_profile == audio::LatencyTunerProfile_Responsive) {
            // Responsive profile requires faster reactions to network changes.
            sliding_window_length = 10000;
        } else {
            sliding_window_length = 30000;
        }
    }

    return true;
}

LinkMeter::LinkMeter(packet::IWriter& writer,
                     const LinkMeterConfig& config,
                     const EncodingMap& encoding_map,
                     core::IArena& arena,
                     dbgio::CsvDumper* dumper)
    : encoding_map_(encoding_map)
    , encoding_(NULL)
    , writer_(writer)
    , first_packet_(true)
    , win_len_(config.sliding_window_length)
    , has_metrics_(false)
    , first_seqnum_(0)
    , last_seqnum_hi_(0)
    , last_seqnum_lo_(0)
    , processed_packets_(0)
    , prev_queue_timestamp_(-1)
    , prev_stream_timestamp_(0)
    , packet_jitter_stats_(arena, win_len_)
    , dumper_(dumper) {
}

status::StatusCode LinkMeter::init_status() const {
    return status::StatusOK;
}

bool LinkMeter::has_metrics() const {
    return has_metrics_;
}

const packet::LinkMetrics& LinkMeter::metrics() const {
    return metrics_;
}

bool LinkMeter::has_encoding() const {
    return encoding_ != NULL;
}

const Encoding& LinkMeter::encoding() const {
    if (encoding_ == NULL) {
        roc_panic("link meter: encoding not available");
    }

    return *encoding_;
}

void LinkMeter::process_report(const rtcp::SendReport& report) {
    // Currently LinkMeter calculates all link metrics except RTT, and
    // RTT is calculated by RTCP module and passed here.
    metrics_.rtt = report.rtt;
}

status::StatusCode LinkMeter::write(const packet::PacketPtr& packet) {
    if (!packet) {
        roc_panic("link meter: null packet");
    }

    // When we create LinkMeter, we don't know yet if RTP is used (e.g.
    // for repair packets), so we should be ready for non-rtp packets.
    if (packet->has_flags(packet::Packet::FlagRTP | packet::Packet::FlagUDP)) {
        // Since we don't know packet type in-before, we also determine
        // encoding dynamically.
        if (!encoding_ || encoding_->payload_type != packet->rtp()->payload_type) {
            encoding_ = encoding_map_.find_by_pt(packet->rtp()->payload_type);
        }
        if (encoding_) {
            update_metrics_(*packet);
        }
    }

    return writer_.write(packet);
}

void LinkMeter::update_metrics_(const packet::Packet& packet) {
    update_seqnums_(packet);

    if (!first_packet_) {
        update_jitter_(packet);
    } else {
        first_packet_ = false;
    }

    processed_packets_++;

    prev_queue_timestamp_ = packet.udp()->queue_timestamp;
    prev_stream_timestamp_ = packet.rtp()->stream_timestamp;

    has_metrics_ = true;
}

void LinkMeter::update_seqnums_(const packet::Packet& packet) {
    const packet::seqnum_t pkt_seqnum = packet.rtp()->seqnum;

    // If packet seqnum is before first seqnum, and there was no wrap yet,
    // update first seqnum.
    if ((first_packet_ || packet::seqnum_diff(pkt_seqnum, first_seqnum_) < 0)
        && last_seqnum_hi_ == 0) {
        first_seqnum_ = pkt_seqnum;
    }

    if (first_packet_) {
        last_seqnum_hi_ = 0;
        last_seqnum_lo_ = pkt_seqnum;
    } else if (packet::seqnum_diff(pkt_seqnum, last_seqnum_lo_) > 0) {
        // If packet seqnum is after last seqnum, update last seqnum, and count
        // possible wraps.
        if (pkt_seqnum < last_seqnum_lo_) {
            last_seqnum_hi_ += (uint32_t)1 << 16;
        }
        last_seqnum_lo_ = pkt_seqnum;
    }

    metrics_.ext_first_seqnum = first_seqnum_;
    metrics_.ext_last_seqnum = last_seqnum_hi_ + last_seqnum_lo_;
    metrics_.expected_packets = metrics_.ext_last_seqnum - first_seqnum_ + 1;
    metrics_.lost_packets = (int64_t)metrics_.expected_packets - processed_packets_ - 1;
}

void LinkMeter::update_jitter_(const packet::Packet& packet) {
    // Link meter operates before FEC, so we should never see restored packets.
    // Otherwise we'd need to exclude them from jitter calculations.
    roc_panic_if_msg(packet.has_flags(packet::Packet::FlagRestored),
                     "link meter: unexpected packet with restored flag");

    roc_panic_if(!encoding_);
    roc_panic_if(prev_queue_timestamp_ <= 0);

    const core::nanoseconds_t d_enq_ns =
        packet.udp()->queue_timestamp - prev_queue_timestamp_;
    const packet::stream_timestamp_diff_t d_s_ts = packet::stream_timestamp_diff(
        packet.rtp()->stream_timestamp, prev_stream_timestamp_);
    const core::nanoseconds_t d_s_ns =
        encoding_->sample_spec.stream_timestamp_delta_2_ns(d_s_ts);

    packet_jitter_stats_.add(std::abs(d_enq_ns - d_s_ns));
    metrics_.jitter = (core::nanoseconds_t)packet_jitter_stats_.mov_avg();
    metrics_.max_jitter = (core::nanoseconds_t)packet_jitter_stats_.mov_max();
    metrics_.min_jitter = (core::nanoseconds_t)packet_jitter_stats_.mov_min();

    if (dumper_) {
        dump_(packet, d_enq_ns, d_s_ns);
    }
}

core::nanoseconds_t rtp::LinkMeter::mean_jitter() const {
    return (core::nanoseconds_t)packet_jitter_stats_.mov_avg();
}

size_t LinkMeter::running_window_len() const {
    return win_len_;
}

void LinkMeter::dump_(const packet::Packet& packet,
                      const long d_enq_ns,
                      const long d_s_ns) {
    dbgio::CsvEntry e;
    e.type = 'm';
    e.n_fields = 5;
    e.fields[0] = packet.udp()->queue_timestamp;
    e.fields[1] = packet.rtp()->stream_timestamp;
    e.fields[2] = (double)std::abs(d_enq_ns - d_s_ns) / core::Millisecond;
    e.fields[3] = packet_jitter_stats_.mov_max();
    e.fields[4] = packet_jitter_stats_.mov_min();
    dumper_->write(e);
}

} // namespace rtp
} // namespace roc
