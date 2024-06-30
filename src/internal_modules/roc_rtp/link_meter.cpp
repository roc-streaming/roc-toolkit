/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "link_meter.h"
#include "roc_core/panic.h"
#include "roc_packet/units.h"

namespace roc {
namespace rtp {

LinkMeter::LinkMeter(core::IArena& arena,
                     const EncodingMap& encoding_map,
                     const audio::SampleSpec& sample_spec,
                     audio::LatencyConfig latency_config,
                     core::CsvDumper* dumper)
    : encoding_map_(encoding_map)
    , encoding_(NULL)
    , writer_(NULL)
    , reader_(NULL)
    , sample_spec_(sample_spec)
    , first_packet_jitter_(true)
    , win_len_(latency_config.tuner_profile == audio::LatencyTunerProfile_Responsive
                   ? 10000
                   : 30000)
    , has_metrics_(false)
    , first_seqnum_(0)
    , last_seqnum_hi_(0)
    , last_seqnum_lo_(0)
    , processed_packets_(0)
    , prev_packet_enq_ts_(-1)
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
            update_jitter_(*packet);
        }
    }

    return writer_->write(packet);
}

status::StatusCode LinkMeter::read(packet::PacketPtr& packet,
                                   packet::PacketReadMode mode) {
    if (!reader_) {
        roc_panic("link meter: forgot to call set_reader()");
    }

    status::StatusCode result = reader_->read(packet, mode);
    if (packet && packet->has_flags(packet::Packet::FlagRestored)) {
        metrics_.recovered_packets++;
    }

    return result;
}

void LinkMeter::set_writer(packet::IWriter& writer) {
    writer_ = &writer;
}

void LinkMeter::set_reader(packet::IReader& reader) {
    reader_ = &reader;
}

void LinkMeter::update_jitter_(const packet::Packet& packet) {
    // Do not calculate jitter on recovered packets.
    if (packet.has_flags(packet::Packet::FlagRestored)) {
        return;
    }

    const packet::seqnum_t pkt_seqnum = packet.rtp()->seqnum;

    // If packet seqnum is before first seqnum, and there was no wrap yet,
    // update first seqnum.
    if ((first_packet_jitter_ || packet::seqnum_diff(pkt_seqnum, first_seqnum_) < 0)
        && last_seqnum_hi_ == 0) {
        first_seqnum_ = pkt_seqnum;
    }

    if (first_packet_jitter_) {
        last_seqnum_hi_ = 0;
        last_seqnum_lo_ = pkt_seqnum;

        // If packet seqnum is after last seqnum, update last seqnum, and
        // also counts possible wraps.
    } else if (packet::seqnum_diff(pkt_seqnum, last_seqnum_lo_) > 0) {
        if (pkt_seqnum < last_seqnum_lo_) {
            last_seqnum_hi_ += 1 << 16;
        }
        last_seqnum_lo_ = pkt_seqnum;
    }

    if (!first_packet_jitter_) {
        // Compute jitter only on consequential packets.
        const core::nanoseconds_t d_enq_ns =
            packet.udp()->queue_timestamp - prev_packet_enq_ts_;
        const packet::stream_timestamp_diff_t d_s_ts = packet::stream_timestamp_diff(
            packet.rtp()->stream_timestamp, prev_stream_timestamp_);
        const core::nanoseconds_t d_s_ns =
            sample_spec_.stream_timestamp_delta_2_ns(d_s_ts);

        packet_jitter_stats_.add(std::abs(d_enq_ns - d_s_ns));
        metrics_.max_jitter = (core::nanoseconds_t)packet_jitter_stats_.mov_max();
        metrics_.min_jitter = (core::nanoseconds_t)packet_jitter_stats_.mov_min();
        metrics_.jitter = mean_jitter();

        if (dumper_) {
            core::CsvEntry e;
            e.type = 'm';
            e.n_fields = 5;
            e.fields[0] = packet.udp()->queue_timestamp;
            e.fields[1] = packet.rtp()->stream_timestamp;
            e.fields[2] = (double)std::abs(d_enq_ns - d_s_ns) / core::Millisecond;
            e.fields[3] = packet_jitter_stats_.mov_max();
            e.fields[4] = packet_jitter_stats_.mov_min();
            dumper_->write(e);
        }
    } else {
        first_packet_jitter_ = false;
    }

    prev_packet_enq_ts_ = packet.udp()->queue_timestamp;
    prev_stream_timestamp_ = packet.rtp()->stream_timestamp;
    processed_packets_++;

    metrics_.ext_first_seqnum = first_seqnum_;
    metrics_.ext_last_seqnum = last_seqnum_hi_ + last_seqnum_lo_;
    metrics_.total_packets = metrics_.ext_last_seqnum - first_seqnum_ + 1;
    metrics_.lost_packets = (ssize_t)metrics_.total_packets - processed_packets_;

    has_metrics_ = true;
}

core::nanoseconds_t rtp::LinkMeter::mean_jitter() const {
    return (core::nanoseconds_t)packet_jitter_stats_.mov_avg();
}

size_t LinkMeter::running_window_len() const {
    return win_len_;
}

} // namespace rtp
} // namespace roc
