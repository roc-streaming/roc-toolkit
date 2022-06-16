/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <stdio.h>

#include "roc_rtcp/print_packet.h"
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

void print_header(const header::PacketHeader& hdr) {
    fprintf(stderr, "|- header:\n");
    fprintf(stderr, "|-- version: %d\n", (int)hdr.version());
    fprintf(stderr, "|-- padding: %d\n", (int)hdr.has_padding());
    fprintf(stderr, "|-- counter: %d\n", (int)hdr.counter());
    fprintf(stderr, "|-- type: %d\n", (int)hdr.type());
    fprintf(stderr, "|-- length: %d bytes (%d words)\n", (int)hdr.len_bytes(),
            (int)hdr.len_words());
}

void print_reception_block(const header::ReceptionReportBlock& blk) {
    fprintf(stderr, "|- block:\n");
    fprintf(stderr, "|-- ssrc: %lu\n", (unsigned long)blk.ssrc());
    fprintf(stderr, "|-- fract_loss: %f\n", (double)blk.fract_loss());
    fprintf(stderr, "|-- cumloss: %d\n", (int)blk.cumloss());
    fprintf(stderr, "|-- last_seqnum: %lu\n", (unsigned long)blk.last_seqnum());
    fprintf(stderr, "|-- jitter: %lu\n", (unsigned long)blk.jitter());
    fprintf(stderr, "|-- lsr: %lu\n", (unsigned long)blk.last_sr());
    fprintf(stderr, "|-- dlsr: %lu\n", (unsigned long)blk.delay_last_sr());
}

void print_rr(const header::ReceiverReportPacket& rr) {
    fprintf(stderr, "+ rr:\n");

    print_header(rr.header());

    fprintf(stderr, "|- body:\n");
    fprintf(stderr, "|-- ssrc: %lu\n", (unsigned long)rr.ssrc());

    for (size_t n = 0; n < rr.num_blocks(); n++) {
        print_reception_block(rr.get_block(n));
    }
}

void print_sr(const header::SenderReportPacket& sr) {
    fprintf(stderr, "+ sr:\n");

    print_header(sr.header());

    fprintf(stderr, "|- body:\n");
    fprintf(stderr, "|-- ssrc: %lu\n", (unsigned long)sr.ssrc());
    fprintf(stderr, "|-- ntp_timestamp: %llu (0x%llx)\n",
            (unsigned long long)sr.ntp_timestamp(),
            (unsigned long long)sr.ntp_timestamp());
    fprintf(stderr, "|-- rtp_timestamp: %llu\n", (unsigned long long)sr.rtp_timestamp());
    fprintf(stderr, "|-- packet_count: %d\n", (int)sr.packet_count());
    fprintf(stderr, "|-- byte_count: %d\n", (int)sr.byte_count());

    for (size_t n = 0; n < sr.num_blocks(); n++) {
        print_reception_block(sr.get_block(n));
    }
}

void print_xr_block_header(const header::XrBlockHeader& hdr) {
    fprintf(stderr, "|-- block header:\n");
    fprintf(stderr, "|--- type: %d\n", (int)hdr.block_type());
    fprintf(stderr, "|--- type_specific: %d\n", (int)hdr.type_specific());
    fprintf(stderr, "|--- length: %d bytes (%d words)\n", (int)hdr.len_bytes(),
            (int)hdr.len_words());
}

void print_xr_rrtr(const header::XrRrtrBlock& blk) {
    fprintf(stderr, "|- rrtr:\n");

    print_xr_block_header(blk.header());

    fprintf(stderr, "|-- block body:\n");
    fprintf(stderr, "|--- ntp_timestamp: %llu (0x%llx)\n",
            (unsigned long long)blk.ntp_timestamp(),
            (unsigned long long)blk.ntp_timestamp());
}

void print_xr_dlrr(const header::XrDlrrBlock& blk) {
    fprintf(stderr, "|- dlrr:\n");

    print_xr_block_header(blk.header());

    for (size_t n = 0; n < blk.num_subblocks(); n++) {
        const header::XrDlrrSubblock& sub_blk = blk.get_subblock(n);

        fprintf(stderr, "|-- subblock:\n");
        fprintf(stderr, "|--- ssrc: %lu\n", (unsigned long)sub_blk.ssrc());
        fprintf(stderr, "|--- lrr: %lu\n", (unsigned long)sub_blk.last_rr());
        fprintf(stderr, "|--- dlrr: %lu\n", (unsigned long)sub_blk.delay_last_rr());
    }
}

void print_xr(const XrTraverser& xr) {
    fprintf(stderr, "+ xr:\n");

    print_header(xr.packet().header());

    fprintf(stderr, "|- body:\n");
    fprintf(stderr, "|-- ssrc: %lu\n", (unsigned long)xr.packet().ssrc());

    XrTraverser::Iterator iter = xr.iter();
    XrTraverser::Iterator::State state;

    while ((state = iter.next()) != XrTraverser::Iterator::END) {
        switch (state) {
        case XrTraverser::Iterator::BEGIN:
        case XrTraverser::Iterator::END:
            break;

        case XrTraverser::Iterator::RRTR_BLOCK:
            print_xr_rrtr(iter.get_rrtr());
            break;

        case XrTraverser::Iterator::DRLL_BLOCK:
            print_xr_dlrr(iter.get_dlrr());
            break;
        }
    }
}

void print_sdes(const SdesTraverser& sdes) {
    fprintf(stderr, "+ sdes:\n");

    SdesTraverser::Iterator iter = sdes.iter();
    SdesTraverser::Iterator::State state;

    while ((state = iter.next()) != SdesTraverser::Iterator::END) {
        switch (state) {
        case SdesTraverser::Iterator::BEGIN:
        case SdesTraverser::Iterator::END:
            break;

        case SdesTraverser::Iterator::CHUNK: {
            const SdesChunk chunk = iter.chunk();
            fprintf(stderr, "|- chunk:\n");
            fprintf(stderr, "|-- ssrc: %lu\n", (unsigned long)chunk.ssrc);
        } break;

        case SdesTraverser::Iterator::ITEM: {
            const SdesItem item = iter.item();
            fprintf(stderr, "|-- item:\n");
            fprintf(stderr, "|--- type: %s (%d)\n", item_type_to_str(item.type),
                    (int)item.type);
            fprintf(stderr, "|--- text: %s\n", item.text);
        } break;
        }
    }
}

void print_bye(const ByeTraverser& bye) {
    fprintf(stderr, "+ bye:\n");

    ByeTraverser::Iterator iter = bye.iter();
    ByeTraverser::Iterator::Iterator::State state;

    while ((state = iter.next()) != ByeTraverser::Iterator::END) {
        switch (state) {
        case ByeTraverser::Iterator::BEGIN:
        case ByeTraverser::Iterator::END:
            break;

        case ByeTraverser::Iterator::SSRC:
            fprintf(stderr, "|- ssrc: %lu\n", (unsigned long)iter.ssrc());
            break;

        case ByeTraverser::Iterator::REASON:
            fprintf(stderr, "|- reason: %s\n", iter.reason());
            break;
        }
    }
}

} // namespace

void print_packet(const core::Slice<uint8_t>& data) {
    fprintf(stderr, "@ rtcp packet (%d bytes)\n", (int)data.size());

    Traverser traverser(data);
    if (!traverser.parse()) {
        fprintf(stderr, "+ <invalid>\n");
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
            print_rr(iter.get_rr());
        } break;

        case Traverser::Iterator::SR: {
            print_sr(iter.get_sr());
        } break;

        case Traverser::Iterator::XR: {
            XrTraverser xr = iter.get_xr();
            if (!xr.parse()) {
                fprintf(stderr, "+ xr:\n|- <invalid>\n");
                break;
            }
            print_xr(xr);
        } break;

        case Traverser::Iterator::SDES: {
            SdesTraverser sdes = iter.get_sdes();
            if (!sdes.parse()) {
                fprintf(stderr, "+ sdes:\n|- <invalid>\n");
                break;
            }
            print_sdes(sdes);
        } break;

        case Traverser::Iterator::BYE:
            ByeTraverser bye = iter.get_bye();
            if (!bye.parse()) {
                fprintf(stderr, "+ bye:\n|- <invalid>\n");
                break;
            }
            print_bye(bye);
        }
    }
}

} // namespace rtcp
} // namespace roc
