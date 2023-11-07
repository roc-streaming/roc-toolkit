/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/session.h"
#include "roc_core/fast_random.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"
#include "roc_packet/ntp.h"

namespace roc {
namespace rtcp {

Session::Session(IReceiverHooks* recv_hooks,
                 ISenderHooks* send_hooks,
                 packet::IWriter* packet_writer,
                 packet::IComposer& packet_composer,
                 packet::PacketFactory& packet_factory,
                 core::BufferFactory<uint8_t>& buffer_factory)
    : packet_factory_(packet_factory)
    , buffer_factory_(buffer_factory)
    , packet_writer_(packet_writer)
    , packet_composer_(packet_composer)
    , recv_hooks_(recv_hooks)
    , send_hooks_(send_hooks)
    , next_deadline_(0)
    , ssrc_(0)
    , valid_(false) {
    ssrc_ =
        (packet::stream_source_t)core::fast_random_range(0, packet::stream_source_t(-1));

    // TODO(gh-14): fill cname
    strcpy(cname_, "TODO");

    roc_log(LogDebug,
            "rtcp session: initialized: is_sender=%d is_receiver=%d ssrc=%lu cname=%s",
            !!(send_hooks_ != NULL), !!(recv_hooks_ != NULL), (unsigned long)ssrc_,
            cname_);

    valid_ = true;
}

bool Session::is_valid() const {
    return valid_;
}

status::StatusCode Session::process_packet(const packet::PacketPtr& packet) {
    roc_panic_if_msg(!packet, "rtcp session: null packet");
    roc_panic_if_msg(!packet->rtcp(), "rtcp session: non-rtcp packet");

    Traverser traverser(packet->rtcp()->data);
    if (!traverser.parse()) {
        roc_log(LogTrace, "rtcp session: can't parse rtcp packet");
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    parse_events_(traverser);
    parse_reports_(traverser);

    return status::StatusOK;
}

core::nanoseconds_t Session::generation_deadline(core::nanoseconds_t current_time) {
    if (next_deadline_ == 0) {
        // until generate_packets() is called first time, report that
        // we're ready immediately
        next_deadline_ = current_time;
    }

    return next_deadline_;
}

status::StatusCode Session::generate_packets(core::nanoseconds_t current_time) {
    roc_panic_if_msg(!packet_writer_, "rtcp session: packet writer not set");

    if (next_deadline_ == 0) {
        next_deadline_ = current_time;
    }

    if (next_deadline_ > current_time) {
        return status::StatusOK;
    }

    do {
        // TODO(gh-14): use IntervalComputer
        next_deadline_ += core::Millisecond * 200;
    } while (next_deadline_ <= current_time);

    packet::PacketPtr packet;
    const status::StatusCode code = generate_packet_(current_time, packet);
    if (code != status::StatusOK) {
        return code;
    }

    return packet_writer_->write(packet);
}

void Session::parse_events_(const Traverser& traverser) {
    Traverser::Iterator iter = traverser.iter();
    Traverser::Iterator::State state;

    while ((state = iter.next()) != Traverser::Iterator::END) {
        switch (state) {
        case Traverser::Iterator::SDES: {
            SdesTraverser sdes = iter.get_sdes();
            if (!sdes.parse()) {
                roc_log(LogTrace, "rtcp session: can't parse sdes packet");
                break;
            }
            parse_session_description_(sdes);
        } break;

        case Traverser::Iterator::BYE: {
            ByeTraverser bye = iter.get_bye();
            if (!bye.parse()) {
                roc_log(LogTrace, "rtcp session: can't parse bye packet");
                break;
            }
            parse_goodbye_(bye);
        } break;

        default:
            break;
        }
    }
}

void Session::parse_reports_(const Traverser& traverser) {
    Traverser::Iterator iter = traverser.iter();
    Traverser::Iterator::State state;

    while ((state = iter.next()) != Traverser::Iterator::END) {
        switch (state) {
        case Traverser::Iterator::SR: {
            parse_sender_report_(iter.get_sr());
        } break;

        case Traverser::Iterator::RR: {
            parse_receiver_report_(iter.get_rr());
        } break;

        default:
            break;
        }
    }
}

void Session::parse_session_description_(const SdesTraverser& sdes) {
    SdesTraverser::Iterator iter = sdes.iter();
    SdesTraverser::Iterator::State state;

    SdesChunk chunk;

    while ((state = iter.next()) != SdesTraverser::Iterator::END) {
        switch (state) {
        case SdesTraverser::Iterator::CHUNK: {
            chunk = iter.chunk();
        } break;

        case SdesTraverser::Iterator::ITEM: {
            const SdesItem item = iter.item();
            if (item.type != header::SDES_CNAME) {
                continue;
            }

            if (recv_hooks_) {
                recv_hooks_->on_update_source(chunk.ssrc, item.text);
            }
        } break;

        default:
            break;
        }
    }
}

void Session::parse_goodbye_(const ByeTraverser& bye) {
    ByeTraverser::Iterator iter = bye.iter();
    ByeTraverser::Iterator::Iterator::State state;

    while ((state = iter.next()) != ByeTraverser::Iterator::END) {
        switch (state) {
        case ByeTraverser::Iterator::SSRC:
            if (recv_hooks_) {
                recv_hooks_->on_remove_source(iter.ssrc());
            }
            break;

        default:
            break;
        }
    }
}

void Session::parse_sender_report_(const header::SenderReportPacket& sr) {
    SendingMetrics metrics;
    metrics.origin_time = packet::ntp_2_unix(sr.ntp_timestamp());
    metrics.origin_rtp = sr.rtp_timestamp();

    if (recv_hooks_) {
        recv_hooks_->on_add_sending_metrics(metrics);
    }

    for (size_t n = 0; n < sr.num_blocks(); n++) {
        parse_reception_block_(sr.get_block(n));
    }
}

void Session::parse_receiver_report_(const header::ReceiverReportPacket& rr) {
    for (size_t n = 0; n < rr.num_blocks(); n++) {
        parse_reception_block_(rr.get_block(n));
    }
}

void Session::parse_reception_block_(const header::ReceptionReportBlock& blk) {
    ReceptionMetrics metrics;
    metrics.ssrc = blk.ssrc();
    metrics.fract_loss = blk.fract_loss();

    if (send_hooks_) {
        send_hooks_->on_add_reception_metrics(metrics);
    }
}

status::StatusCode Session::generate_packet_(core::nanoseconds_t current_time,
                                             packet::PacketPtr& packet) {
    packet = packet_factory_.new_packet();
    if (!packet) {
        roc_log(LogError, "rtcp session: can't create packet");
        // TODO(gh-183): return StatusNoMem
        return status::StatusOK;
    }

    // will hold composed RTCP packet
    core::Slice<uint8_t> rtcp_data = buffer_factory_.new_buffer();
    if (!rtcp_data) {
        roc_log(LogError, "rtcp session: can't create buffer");
        // TODO(gh-183): return StatusNoMem
        return status::StatusOK;
    }

    // reset slice
    rtcp_data.reslice(0, 0);

    // fill RTCP packet
    build_packet_(rtcp_data, current_time);

    // will hold whole packet data; if RTCP composer is nested into another
    // composer, packet_data may hold additionals headers or footers around
    // RTCP; if RTCP composer is the topmost, packet_data and rtcp_data
    // will be identical
    core::Slice<uint8_t> packet_data = buffer_factory_.new_buffer();
    if (!packet_data) {
        roc_log(LogError, "rtcp session: can't create buffer");
        // TODO(gh-183): return StatusNoMem
        return status::StatusOK;
    }

    // reset slice
    packet_data.reslice(0, 0);

    // prepare packet to be able to hold our RTCP packet
    if (!packet_composer_.prepare(*packet, packet_data, rtcp_data.size())) {
        roc_log(LogError, "rtcp session: can't prepare packet");
        // TODO(gh-183): return status
        return status::StatusOK;
    }
    packet->add_flags(packet::Packet::FlagPrepared);

    // attach prepared packet data to the packet
    packet->set_data(packet_data);

    // prepare() call should have, among other things, set packet->rtcp()->data to a
    // sub-slice of packet_data, of size exactly as we requested
    if (!packet->rtcp() || !packet->rtcp()->data
        || packet->rtcp()->data.size() != rtcp_data.size()) {
        roc_log(LogError, "rtcp session: composer prepared invalid packet");
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    // copy our RTCP packet into that sub-slice
    memcpy(packet->rtcp()->data.data(), rtcp_data.data(), rtcp_data.size());

    return status::StatusOK;
}

void Session::build_packet_(core::Slice<uint8_t>& data, core::nanoseconds_t report_time) {
    Builder bld(data);

    if (send_hooks_) {
        // if we're sending and probably also receiving
        build_sender_report_(bld, report_time);
    } else {
        // if we're only receiving
        build_receiver_report_(bld, report_time);
    }

    build_session_description_(bld);
}

void Session::build_sender_report_(Builder& bld, core::nanoseconds_t report_time) {
    roc_panic_if(!send_hooks_);

    const SendingMetrics metrics = send_hooks_->on_get_sending_metrics(report_time);

    header::SenderReportPacket sr;
    sr.set_ssrc(ssrc_);
    sr.set_ntp_timestamp(packet::unix_2_ntp(metrics.origin_time));
    sr.set_rtp_timestamp(metrics.origin_rtp);

    bld.begin_sr(sr);

    if (recv_hooks_) {
        const size_t num_sources = recv_hooks_->on_get_num_sources();

        for (size_t n = 0; n < num_sources; n++) {
            bld.add_sr_report(
                build_reception_block_(recv_hooks_->on_get_reception_metrics(n)));
        }
    }

    bld.end_sr();
}

void Session::build_receiver_report_(Builder& bld, core::nanoseconds_t report_time) {
    header::ReceiverReportPacket rr;
    rr.set_ssrc(ssrc_);

    bld.begin_rr(rr);

    if (recv_hooks_) {
        const size_t num_sources = recv_hooks_->on_get_num_sources();

        for (size_t n = 0; n < num_sources; n++) {
            bld.add_rr_report(
                build_reception_block_(recv_hooks_->on_get_reception_metrics(n)));
        }
    }

    bld.end_rr();

    header::XrPacket xr;
    xr.set_ssrc(ssrc_);

    bld.begin_xr(xr);

    {
        header::XrRrtrBlock rrtr;
        rrtr.set_ntp_timestamp(packet::unix_2_ntp(report_time));

        bld.add_xr_rrtr(rrtr);
    }

    bld.end_xr();
}

header::ReceptionReportBlock
Session::build_reception_block_(const ReceptionMetrics& metrics) {
    header::ReceptionReportBlock blk;

    blk.set_ssrc(metrics.ssrc);

    return blk;
}

void Session::build_session_description_(Builder& bld) {
    bld.begin_sdes();

    build_source_description_(bld, ssrc_);

    if (send_hooks_) {
        const size_t num_sources = send_hooks_->on_get_num_sources();

        for (size_t n = 0; n < num_sources; n++) {
            build_source_description_(bld, send_hooks_->on_get_sending_source(n));
        }
    }

    bld.end_sdes();
}

void Session::build_source_description_(Builder& bld, packet::stream_source_t ssrc) {
    SdesChunk chunk;
    chunk.ssrc = ssrc;

    bld.begin_sdes_chunk(chunk);

    {
        SdesItem item;
        item.type = header::SDES_CNAME;
        item.text = cname_;

        bld.add_sdes_item(item);
    }

    bld.end_sdes_chunk();
}

} // namespace rtcp
} // namespace roc
