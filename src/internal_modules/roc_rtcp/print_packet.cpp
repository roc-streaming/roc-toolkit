/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/print_packet.h"
#include "roc_core/printer.h"
#include "roc_packet/ntp.h"
#include "roc_rtcp/headers.h"
#include "roc_rtcp/traverser.h"

namespace roc {
namespace rtcp {

namespace {

const char* item_type_to_str(header::SdesItemType t) {
    switch (t) {
    case header::SDES_CNAME:
        return "cname";
    case header::SDES_NAME:
        return "name";
    case header::SDES_EMAIL:
        return "email";
    case header::SDES_PHONE:
        return "phone";
    case header::SDES_LOC:
        return "loc";
    case header::SDES_TOOL:
        return "tool";
    case header::SDES_NOTE:
        return "note";
    case header::SDES_PRIV:
        return "priv";
    }

    return "?";
}

void print_header(core::Printer& p, const header::PacketHeader& hdr) {
    p.writef("|- header:\n");
    p.writef("|-- version: %d\n", (int)hdr.version());
    p.writef("|-- padding: %d\n", (int)hdr.has_padding());
    p.writef("|-- counter: %d\n", (int)hdr.counter());
    p.writef("|-- type: %d\n", (int)hdr.type());
    p.writef("|-- length: %d bytes (%d words)\n", (int)hdr.len_bytes(),
             (int)hdr.len_words());
}

void print_reception_block(core::Printer& p, const header::ReceptionReportBlock& blk) {
    p.writef("|- block:\n");
    p.writef("|-- ssrc: %lu\n", (unsigned long)blk.ssrc());
    p.writef("|-- fract_loss: %f\n", (double)blk.fract_loss());
    p.writef("|-- cum_loss: %d\n", (int)blk.cum_loss());
    p.writef("|-- last_seqnum: %lu\n", (unsigned long)blk.last_seqnum());
    p.writef("|-- jitter: %lu\n", (unsigned long)blk.jitter());
    p.writef("|-- lsr: %016llx (unix %lld)\n", (unsigned long long)blk.last_sr(),
             (long long)packet::ntp_2_unix(blk.last_sr()));
    p.writef("|-- dlsr: %016llx (unix %lld)\n", (unsigned long long)blk.delay_last_sr(),
             (long long)packet::ntp_2_nanoseconds(blk.delay_last_sr()));
}

void print_rr(core::Printer& p, const header::ReceiverReportPacket& rr) {
    p.writef("+ rr:\n");

    print_header(p, rr.header());

    p.writef("|- body:\n");
    p.writef("|-- ssrc: %lu\n", (unsigned long)rr.ssrc());

    for (size_t n = 0; n < rr.num_blocks(); n++) {
        print_reception_block(p, rr.get_block(n));
    }
}

void print_sr(core::Printer& p, const header::SenderReportPacket& sr) {
    p.writef("+ sr:\n");

    print_header(p, sr.header());

    p.writef("|- body:\n");
    p.writef("|-- ssrc: %lu\n", (unsigned long)sr.ssrc());
    p.writef("|-- ntp_timestamp: %016llx (unix %lld)\n",
             (unsigned long long)sr.ntp_timestamp(),
             (long long)packet::ntp_2_unix(sr.ntp_timestamp()));
    p.writef("|-- rtp_timestamp: %lu\n", (unsigned long)sr.rtp_timestamp());
    p.writef("|-- packet_count: %d\n", (int)sr.packet_count());
    p.writef("|-- byte_count: %d\n", (int)sr.byte_count());

    for (size_t n = 0; n < sr.num_blocks(); n++) {
        print_reception_block(p, sr.get_block(n));
    }
}

void print_xr_block_header(core::Printer& p, const header::XrBlockHeader& hdr) {
    p.writef("|-- block header:\n");
    p.writef("|--- type: %d\n", (int)hdr.block_type());
    p.writef("|--- type_specific: 0x%x\n", (unsigned)hdr.type_specific());
    p.writef("|--- length: %d bytes (%d words)\n", (int)hdr.len_bytes(),
             (int)hdr.len_words());
}

void print_xr_rrtr(core::Printer& p, const header::XrRrtrBlock& blk) {
    p.writef("|- rrtr:\n");

    print_xr_block_header(p, blk.header());

    p.writef("|-- block body:\n");
    p.writef("|--- ntp_timestamp: %016llx (unix %lld)\n",
             (unsigned long long)blk.ntp_timestamp(),
             (long long)packet::ntp_2_unix(blk.ntp_timestamp()));
}

void print_xr_dlrr(core::Printer& p, const header::XrDlrrBlock& blk) {
    p.writef("|- dlrr:\n");

    print_xr_block_header(p, blk.header());

    for (size_t n = 0; n < blk.num_subblocks(); n++) {
        const header::XrDlrrSubblock& sub_blk = blk.get_subblock(n);

        p.writef("|-- subblock:\n");
        p.writef("|--- ssrc: %lu\n", (unsigned long)sub_blk.ssrc());
        p.writef("|--- lrr: %016llx (unix %lld)\n", (unsigned long long)sub_blk.last_rr(),
                 (long long)packet::ntp_2_unix(sub_blk.last_rr()));
        p.writef("|--- dlrr: %016llx (unix %lld)\n",
                 (unsigned long long)sub_blk.delay_last_rr(),
                 (long long)packet::ntp_2_nanoseconds(sub_blk.delay_last_rr()));
    }
}

void print_xr_measurement_info(core::Printer& p,
                               const header::XrMeasurementInfoBlock& blk) {
    p.writef("|- measurement:\n");

    print_xr_block_header(p, blk.header());

    p.writef("|-- block body:\n");
    p.writef("|--- ssrc: %lu\n", (unsigned long)blk.ssrc());
    p.writef("|--- first_sn: %lu\n", (unsigned long)blk.first_seq());
    p.writef("|--- interval_first_sn: %lu\n", (unsigned long)blk.interval_first_seq());
    p.writef("|--- interval_last_sn: %lu\n", (unsigned long)blk.interval_last_seq());
    p.writef("|--- interval_duration: %016llx (unix %lld)\n",
             (unsigned long long)blk.interval_duration(),
             (long long)packet::ntp_2_nanoseconds(blk.interval_duration()));
    p.writef("|--- cum_duration: %016llx (unix %lld)\n",
             (unsigned long long)blk.cum_duration(),
             (long long)packet::ntp_2_nanoseconds(blk.cum_duration()));
}

void print_metric_flag(core::Printer& p, const header::MetricFlag flag) {
    switch (flag) {
    case header::MetricFlag_IntervalDuration:
        p.writef("|--- flag: interval (%d)\n", flag);
        break;
    case header::MetricFlag_CumulativeDuration:
        p.writef("|--- flag: cumulative (%d)\n", flag);
        break;
    case header::MetricFlag_SampledValue:
        p.writef("|--- flag: sample (%d)\n", flag);
        break;
    default:
        p.writef("|--- flag: unknown (%d)\n", flag);
        break;
    }
}

void print_xr_delay_metrics(core::Printer& p, const header::XrDelayMetricsBlock& blk) {
    p.writef("|- delay:\n");

    print_xr_block_header(p, blk.header());

    p.writef("|-- block body:\n");
    print_metric_flag(p, blk.metric_flag());
    p.writef("|--- ssrc: %lu\n", (unsigned long)blk.ssrc());
    p.writef("|--- rtt_mean: %016llx (unix %lld)\n", (unsigned long long)blk.mean_rtt(),
             (long long)packet::ntp_2_nanoseconds(blk.mean_rtt()));
    p.writef("|--- rtt_min: %016llx (unix %lld)\n", (unsigned long long)blk.min_rtt(),
             (long long)packet::ntp_2_nanoseconds(blk.min_rtt()));
    p.writef("|--- rtt_max: %016llx (unix %lld)\n", (unsigned long long)blk.max_rtt(),
             (long long)packet::ntp_2_nanoseconds(blk.max_rtt()));
    p.writef("|--- e2e_delay: %016llx (unix %lld)\n", (unsigned long long)blk.e2e_delay(),
             (long long)packet::ntp_2_nanoseconds(blk.e2e_delay()));
}

void print_xr_queue_metrics(core::Printer& p, const header::XrQueueMetricsBlock& blk) {
    p.writef("|- queue:\n");

    print_xr_block_header(p, blk.header());

    p.writef("|-- block body:\n");
    print_metric_flag(p, blk.metric_flag());
    p.writef("|--- ssrc: %lu\n", (unsigned long)blk.ssrc());
    p.writef("|--- niq_delay: %016llx (unix %lld)\n", (unsigned long long)blk.niq_delay(),
             (long long)packet::ntp_2_nanoseconds(blk.niq_delay()));
}

void print_xr(core::Printer& p, const XrTraverser& xr) {
    p.writef("+ xr:\n");

    print_header(p, xr.packet().header());

    p.writef("|- body:\n");
    p.writef("|-- ssrc: %lu\n", (unsigned long)xr.packet().ssrc());

    XrTraverser::Iterator iter = xr.iter();
    XrTraverser::Iterator::State state;

    while ((state = iter.next()) != XrTraverser::Iterator::END) {
        switch (state) {
        case XrTraverser::Iterator::BEGIN:
        case XrTraverser::Iterator::END:
            break;

        case XrTraverser::Iterator::RRTR_BLOCK:
            print_xr_rrtr(p, iter.get_rrtr());
            break;

        case XrTraverser::Iterator::DLRR_BLOCK:
            print_xr_dlrr(p, iter.get_dlrr());
            break;

        case XrTraverser::Iterator::MEASUREMENT_INFO_BLOCK:
            print_xr_measurement_info(p, iter.get_measurement_info());
            break;

        case XrTraverser::Iterator::DELAY_METRICS_BLOCK:
            print_xr_delay_metrics(p, iter.get_delay_metrics());
            break;

        case XrTraverser::Iterator::QUEUE_METRICS_BLOCK:
            print_xr_queue_metrics(p, iter.get_queue_metrics());
            break;
        }
    }
}

void print_sdes(core::Printer& p, const SdesTraverser& sdes) {
    p.writef("+ sdes:\n");

    SdesTraverser::Iterator iter = sdes.iter();
    SdesTraverser::Iterator::State state;

    while ((state = iter.next()) != SdesTraverser::Iterator::END) {
        switch (state) {
        case SdesTraverser::Iterator::BEGIN:
        case SdesTraverser::Iterator::END:
            break;

        case SdesTraverser::Iterator::CHUNK: {
            const SdesChunk chunk = iter.get_chunk();
            p.writef("|- chunk:\n");
            p.writef("|-- ssrc: %lu\n", (unsigned long)chunk.ssrc);
        } break;

        case SdesTraverser::Iterator::ITEM: {
            const SdesItem item = iter.get_item();
            p.writef("|-- item:\n");
            p.writef("|--- type: %s (%d)\n", item_type_to_str(item.type), (int)item.type);
            p.writef("|--- text: %s\n", item.text);
        } break;
        }
    }
}

void print_bye(core::Printer& p, const ByeTraverser& bye) {
    p.writef("+ bye:\n");

    ByeTraverser::Iterator iter = bye.iter();
    ByeTraverser::Iterator::Iterator::State state;

    while ((state = iter.next()) != ByeTraverser::Iterator::END) {
        switch (state) {
        case ByeTraverser::Iterator::BEGIN:
        case ByeTraverser::Iterator::END:
            break;

        case ByeTraverser::Iterator::SSRC:
            p.writef("|- ssrc: %lu\n", (unsigned long)iter.get_ssrc());
            break;

        case ByeTraverser::Iterator::REASON:
            p.writef("|- reason: %s\n", iter.get_reason());
            break;
        }
    }
}

} // namespace

void print_packet(const core::Slice<uint8_t>& data) {
    core::Printer p;

    p.writef("@ rtcp packet (%d bytes)\n", (int)data.size());

    Traverser traverser(data);
    if (!traverser.parse()) {
        p.writef("+ <invalid>\n");
        return;
    }

    Traverser::Iterator iter = traverser.iter();
    Traverser::Iterator::State state;

    while ((state = iter.next()) != Traverser::Iterator::END) {
        switch (state) {
        case Traverser::Iterator::BEGIN:
        case Traverser::Iterator::END:
            break;

        case Traverser::Iterator::RR: {
            print_rr(p, iter.get_rr());
        } break;

        case Traverser::Iterator::SR: {
            print_sr(p, iter.get_sr());
        } break;

        case Traverser::Iterator::XR: {
            XrTraverser xr = iter.get_xr();
            if (!xr.parse()) {
                p.writef("+ xr:\n|- <invalid>\n");
                break;
            }
            print_xr(p, xr);
        } break;

        case Traverser::Iterator::SDES: {
            SdesTraverser sdes = iter.get_sdes();
            if (!sdes.parse()) {
                p.writef("+ sdes:\n|- <invalid>\n");
                break;
            }
            print_sdes(p, sdes);
        } break;

        case Traverser::Iterator::BYE:
            ByeTraverser bye = iter.get_bye();
            if (!bye.parse()) {
                p.writef("+ bye:\n|- <invalid>\n");
                break;
            }
            print_bye(p, bye);
        }
    }
}

} // namespace rtcp
} // namespace roc
