/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_rtcp/builder.h"
#include "roc_rtcp/traverser.h"

namespace roc {
namespace rtcp {
namespace {

enum { MaxBufSize = 1492 };

core::HeapArena arena;
core::BufferFactory<uint8_t> buffer_factory(arena, MaxBufSize);

core::Slice<uint8_t> new_buffer() {
    core::Slice<uint8_t> buff = buffer_factory.new_buffer();
    CHECK(buff);
    return buff.subslice(0, 0);
}

void validate_buffer(const core::Slice<uint8_t>& buff) {
    // RFC 3550, Page 82:
    //
    // o  RTP version field must equal 2.
    //
    // o  The payload type field of the first RTCP packet in a compound
    //    packet must be equal to SR or RR.
    //
    // o  The padding bit (P) should be zero for the first packet of a
    //    compound RTCP packet because padding should only be applied, if it
    //    is needed, to the last packet.
    //
    // o  The length fields of the individual RTCP packets must add up to
    //    the overall length of the compound RTCP packet as received. This
    //    is a fairly strong check.

    CHECK(buff.size() >= sizeof(header::PacketHeader));

    size_t offset = 0;
    size_t pkt_index = 0;

    for (;;) {
        const header::PacketHeader& header = *(const header::PacketHeader*)&buff[offset];

        CHECK(header.version() == header::V2);

        CHECK(header.type() == header::RTCP_SR || header.type() == header::RTCP_RR
              || header.type() == header::RTCP_XR || header.type() == header::RTCP_SDES
              || header.type() == header::RTCP_BYE);

        if (pkt_index == 0) {
            // First packet should be SR or RR.
            CHECK(header.type() == header::RTCP_SR || header.type() == header::RTCP_RR);
        }

        offset += header.len_bytes();
        // Boundary check.
        CHECK(offset <= buff.size());
        // Each packet should be 4-byte aligned.
        CHECK((offset & 0x03) == 0);

        if (offset == buff.size()) {
            break; // Last packet.
        }

        // Only last packet can have padding.
        CHECK(!header.has_padding());

        pkt_index++;
    }
}

} // namespace

TEST_GROUP(builder_traverser) {};

TEST(builder_traverser, sr_sdes) {
    core::Slice<uint8_t> buff = new_buffer();

    header::SenderReportPacket sr;
    sr.set_ssrc(1);
    sr.set_ntp_timestamp(2);
    sr.set_rtp_timestamp(3);
    sr.set_packet_count(4);
    sr.set_byte_count(5);

    header::ReceptionReportBlock sender_report1;
    sender_report1.set_ssrc(1);
    sender_report1.set_fract_loss(1, 8);
    sender_report1.set_cumloss(2);
    sender_report1.set_last_seqnum(3);
    sender_report1.set_jitter(4);
    sender_report1.set_last_sr(5);
    sender_report1.set_delay_last_sr(6);
    header::ReceptionReportBlock sender_report2;
    sender_report2.set_ssrc(1 + 10);
    sender_report2.set_fract_loss(2, 32);
    sender_report2.set_cumloss(2 + 10);
    sender_report2.set_last_seqnum(3 + 10);
    sender_report2.set_jitter(4 + 10);
    sender_report2.set_last_sr(5 + 10);
    sender_report2.set_delay_last_sr(6 + 10);

    // Synthesize part

    Builder builder(buff);

    // SR
    builder.begin_sr(sr);
    builder.add_sr_report(sender_report1);
    builder.add_sr_report(sender_report2);
    builder.end_sr();

    // SDES
    builder.begin_sdes();
    SdesChunk sdes_chunk;
    sdes_chunk.ssrc = 666;
    builder.begin_sdes_chunk(sdes_chunk);
    SdesItem sdes_item_send;
    const char* cname = "1234:cname1";
    sdes_item_send.type = header::SDES_CNAME;
    sdes_item_send.text = cname;
    builder.add_sdes_item(sdes_item_send);
    builder.end_sdes_chunk();
    builder.end_sdes();

    // Validation part

    validate_buffer(buff);

    // Parsing part

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::SR, it.next());
    CHECK_EQUAL(sr.ssrc(), it.get_sr().ssrc());
    CHECK_EQUAL(sr.ntp_timestamp(), it.get_sr().ntp_timestamp());
    CHECK_EQUAL(sr.rtp_timestamp(), it.get_sr().rtp_timestamp());
    CHECK_EQUAL(sr.packet_count(), it.get_sr().packet_count());
    CHECK_EQUAL(sr.byte_count(), it.get_sr().byte_count());
    CHECK_EQUAL(sender_report1.ssrc(), it.get_sr().get_block(0).ssrc());
    DOUBLES_EQUAL(sender_report1.fract_loss(), it.get_sr().get_block(0).fract_loss(),
                  1e-8);
    CHECK_EQUAL(sender_report1.cumloss(), it.get_sr().get_block(0).cumloss());
    CHECK_EQUAL(sender_report1.last_seqnum(), it.get_sr().get_block(0).last_seqnum());
    CHECK_EQUAL(sender_report1.jitter(), it.get_sr().get_block(0).jitter());
    CHECK_EQUAL(sender_report1.last_sr(), it.get_sr().get_block(0).last_sr());
    CHECK_EQUAL(sender_report1.delay_last_sr(), it.get_sr().get_block(0).delay_last_sr());
    CHECK_EQUAL(sender_report2.ssrc(), it.get_sr().get_block(1).ssrc());
    DOUBLES_EQUAL(sender_report2.fract_loss(), it.get_sr().get_block(1).fract_loss(),
                  1e-8);
    CHECK_EQUAL(sender_report2.cumloss(), it.get_sr().get_block(1).cumloss());
    CHECK_EQUAL(sender_report2.last_seqnum(), it.get_sr().get_block(1).last_seqnum());
    CHECK_EQUAL(sender_report2.jitter(), it.get_sr().get_block(1).jitter());
    CHECK_EQUAL(sender_report2.last_sr(), it.get_sr().get_block(1).last_sr());
    CHECK_EQUAL(sender_report2.delay_last_sr(), it.get_sr().get_block(1).delay_last_sr());

    CHECK_EQUAL(Traverser::Iterator::SDES, it.next());
    SdesTraverser sdes = it.get_sdes();
    CHECK(sdes.parse());
    SdesTraverser::Iterator sdes_it = sdes.iter();
    CHECK_EQUAL(1, sdes.chunks_count());

    CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
    SdesChunk sdes_chunk_recv = sdes_it.get_chunk();
    CHECK_EQUAL(666, sdes_chunk_recv.ssrc);

    CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
    SdesItem sdes_item_recv = sdes_it.get_item();
    CHECK_EQUAL(sdes_item_send.type, sdes_item_recv.type);
    STRCMP_EQUAL(sdes_item_send.text, sdes_item_recv.text);
    STRCMP_EQUAL(cname, sdes_item_recv.text);
    CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
    CHECK_FALSE(sdes_it.error());

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(builder_traverser, rr_sdes) {
    core::Slice<uint8_t> buff = new_buffer();

    header::ReceiverReportPacket rr;
    rr.set_ssrc(1);

    header::ReceptionReportBlock receiver_report_1;
    receiver_report_1.set_ssrc(1);
    receiver_report_1.set_fract_loss(1, 8);
    receiver_report_1.set_cumloss(2);
    receiver_report_1.set_last_seqnum(3);
    receiver_report_1.set_jitter(4);
    receiver_report_1.set_last_sr(5);
    receiver_report_1.set_delay_last_sr(6);
    header::ReceptionReportBlock receiver_report_2;
    receiver_report_2.set_ssrc(1 + 10);
    receiver_report_2.set_fract_loss(2, 32);
    receiver_report_2.set_cumloss(2 + 10);
    receiver_report_2.set_last_seqnum(3 + 10);
    receiver_report_2.set_jitter(4 + 10);
    receiver_report_2.set_last_sr(5 + 10);
    receiver_report_2.set_delay_last_sr(6 + 10);

    // Synthesize part

    Builder builder(buff);

    // RR
    builder.begin_rr(rr);
    builder.add_rr_report(receiver_report_1);
    builder.add_rr_report(receiver_report_2);
    builder.end_rr();

    // SDES
    builder.begin_sdes();
    SdesChunk sdes_chunk;
    sdes_chunk.ssrc = 666;
    builder.begin_sdes_chunk(sdes_chunk);
    SdesItem sdes_item_send_1;
    const char* cname = "1234:cname1";
    sdes_item_send_1.type = header::SDES_CNAME;
    sdes_item_send_1.text = cname;
    builder.add_sdes_item(sdes_item_send_1);
    SdesItem sdes_item_send_2;
    const char* name = "name name";
    sdes_item_send_2.type = header::SDES_NAME;
    sdes_item_send_2.text = name;
    builder.add_sdes_item(sdes_item_send_2);
    builder.end_sdes_chunk();
    builder.end_sdes();

    // Validation part

    validate_buffer(buff);

    // Parsing part

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::RR, it.next());
    CHECK_EQUAL(rr.ssrc(), it.get_rr().ssrc());

    CHECK_EQUAL(receiver_report_1.ssrc(), it.get_rr().get_block(0).ssrc());
    DOUBLES_EQUAL(receiver_report_1.fract_loss(), it.get_rr().get_block(0).fract_loss(),
                  1e-8);
    CHECK_EQUAL(receiver_report_1.cumloss(), it.get_rr().get_block(0).cumloss());
    CHECK_EQUAL(receiver_report_1.last_seqnum(), it.get_rr().get_block(0).last_seqnum());
    CHECK_EQUAL(receiver_report_1.jitter(), it.get_rr().get_block(0).jitter());
    CHECK_EQUAL(receiver_report_1.last_sr(), it.get_rr().get_block(0).last_sr());
    CHECK_EQUAL(receiver_report_1.delay_last_sr(),
                it.get_rr().get_block(0).delay_last_sr());

    CHECK_EQUAL(receiver_report_2.ssrc(), it.get_rr().get_block(1).ssrc());
    DOUBLES_EQUAL(receiver_report_2.fract_loss(), it.get_rr().get_block(1).fract_loss(),
                  1e-8);
    CHECK_EQUAL(receiver_report_2.cumloss(), it.get_rr().get_block(1).cumloss());
    CHECK_EQUAL(receiver_report_2.last_seqnum(), it.get_rr().get_block(1).last_seqnum());
    CHECK_EQUAL(receiver_report_2.jitter(), it.get_rr().get_block(1).jitter());
    CHECK_EQUAL(receiver_report_2.last_sr(), it.get_rr().get_block(1).last_sr());
    CHECK_EQUAL(receiver_report_2.delay_last_sr(),
                it.get_rr().get_block(1).delay_last_sr());

    CHECK_EQUAL(Traverser::Iterator::SDES, it.next());
    SdesTraverser sdes = it.get_sdes();
    CHECK(sdes.parse());
    SdesTraverser::Iterator sdes_it = sdes.iter();
    CHECK_EQUAL(1, sdes.chunks_count());

    SdesChunk sdes_chunk_recv;
    SdesItem sdes_item_recv;

    CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
    sdes_chunk_recv = sdes_it.get_chunk();
    CHECK_EQUAL(666, sdes_chunk_recv.ssrc);

    CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
    sdes_item_recv = sdes_it.get_item();
    CHECK_EQUAL(sdes_item_send_1.type, sdes_item_recv.type);
    STRCMP_EQUAL(sdes_item_send_1.text, sdes_item_recv.text);

    CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
    sdes_item_recv = sdes_it.get_item();
    CHECK_EQUAL(sdes_item_send_2.type, sdes_item_recv.type);
    STRCMP_EQUAL(sdes_item_send_2.text, sdes_item_recv.text);
    CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
    CHECK_FALSE(sdes_it.error());

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(builder_traverser, rr_sdes_xr) {
    core::Slice<uint8_t> buff = new_buffer();

    header::ReceiverReportPacket rr;
    rr.set_ssrc(1);

    header::ReceptionReportBlock receiver_report_1;
    receiver_report_1.set_ssrc(1);
    receiver_report_1.set_fract_loss(1, 8);
    receiver_report_1.set_cumloss(2);
    receiver_report_1.set_last_seqnum(3);
    receiver_report_1.set_jitter(4);
    receiver_report_1.set_last_sr(5);
    receiver_report_1.set_delay_last_sr(6);
    header::ReceptionReportBlock receiver_report_2;
    receiver_report_2.set_ssrc(1 + 10);
    receiver_report_2.set_fract_loss(2, 32);
    receiver_report_2.set_cumloss(2 + 10);
    receiver_report_2.set_last_seqnum(3 + 10);
    receiver_report_2.set_jitter(4 + 10);
    receiver_report_2.set_last_sr(5 + 10);
    receiver_report_2.set_delay_last_sr(6 + 10);

    header::XrPacket xr;
    xr.set_ssrc(111);
    header::XrRrtrBlock ref_time;
    ref_time.set_ntp_timestamp(0xFFFFFFFFFFFFFFFF);
    header::XrDlrrBlock dlrr;
    header::XrDlrrSubblock dlrr_repblock_1;
    dlrr_repblock_1.set_ssrc(222);
    dlrr_repblock_1.set_delay_last_rr(333);
    dlrr_repblock_1.set_last_rr(444);
    header::XrDlrrSubblock dlrr_repblock_2;
    dlrr_repblock_2.set_ssrc(555);
    dlrr_repblock_2.set_delay_last_rr(666);
    dlrr_repblock_2.set_last_rr(777);

    // Synthesize part

    Builder builder(buff);

    // RR
    builder.begin_rr(rr);
    builder.add_rr_report(receiver_report_1);
    builder.add_rr_report(receiver_report_2);
    builder.end_rr();

    // SDES
    builder.begin_sdes();
    SdesChunk sdes_chunk;
    sdes_chunk.ssrc = 666;
    builder.begin_sdes_chunk(sdes_chunk);
    SdesItem sdes_item_send_1;
    const char* cname = "1234:cname1";
    sdes_item_send_1.type = header::SDES_CNAME;
    sdes_item_send_1.text = cname;
    builder.add_sdes_item(sdes_item_send_1);
    SdesItem sdes_item_send_2;
    const char* name = "name name";
    sdes_item_send_2.type = header::SDES_NAME;
    sdes_item_send_2.text = name;
    builder.add_sdes_item(sdes_item_send_2);
    builder.end_sdes_chunk();
    builder.end_sdes();

    // XR
    builder.begin_xr(xr);
    builder.add_xr_rrtr(ref_time);
    builder.begin_xr_dlrr(dlrr);
    builder.add_xr_dlrr_report(dlrr_repblock_1);
    builder.add_xr_dlrr_report(dlrr_repblock_2);
    builder.end_xr_dlrr();
    builder.end_xr();

    // Validation part

    validate_buffer(buff);

    // Parsing part

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::RR, it.next());
    CHECK_EQUAL(rr.ssrc(), it.get_rr().ssrc());

    CHECK_EQUAL(receiver_report_1.ssrc(), it.get_rr().get_block(0).ssrc());
    DOUBLES_EQUAL(receiver_report_1.fract_loss(), it.get_rr().get_block(0).fract_loss(),
                  1e-8);
    CHECK_EQUAL(receiver_report_1.cumloss(), it.get_rr().get_block(0).cumloss());
    CHECK_EQUAL(receiver_report_1.last_seqnum(), it.get_rr().get_block(0).last_seqnum());
    CHECK_EQUAL(receiver_report_1.jitter(), it.get_rr().get_block(0).jitter());
    CHECK_EQUAL(receiver_report_1.last_sr(), it.get_rr().get_block(0).last_sr());
    CHECK_EQUAL(receiver_report_1.delay_last_sr(),
                it.get_rr().get_block(0).delay_last_sr());
    CHECK_EQUAL(receiver_report_2.ssrc(), it.get_rr().get_block(1).ssrc());
    DOUBLES_EQUAL(receiver_report_2.fract_loss(), it.get_rr().get_block(1).fract_loss(),
                  1e-8);
    CHECK_EQUAL(receiver_report_2.cumloss(), it.get_rr().get_block(1).cumloss());
    CHECK_EQUAL(receiver_report_2.last_seqnum(), it.get_rr().get_block(1).last_seqnum());
    CHECK_EQUAL(receiver_report_2.jitter(), it.get_rr().get_block(1).jitter());
    CHECK_EQUAL(receiver_report_2.last_sr(), it.get_rr().get_block(1).last_sr());
    CHECK_EQUAL(receiver_report_2.delay_last_sr(),
                it.get_rr().get_block(1).delay_last_sr());

    CHECK_EQUAL(Traverser::Iterator::SDES, it.next());
    SdesTraverser sdes = it.get_sdes();
    CHECK(sdes.parse());
    SdesTraverser::Iterator sdes_it = sdes.iter();
    CHECK_EQUAL(1, sdes.chunks_count());

    SdesChunk sdes_chunk_recv;
    SdesItem sdes_item_recv;

    CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
    sdes_chunk_recv = sdes_it.get_chunk();
    CHECK_EQUAL(666, sdes_chunk_recv.ssrc);

    CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
    sdes_item_recv = sdes_it.get_item();
    CHECK_EQUAL(sdes_item_send_1.type, sdes_item_recv.type);
    STRCMP_EQUAL(sdes_item_send_1.text, sdes_item_recv.text);

    CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
    sdes_item_recv = sdes_it.get_item();
    CHECK_EQUAL(sdes_item_send_2.type, sdes_item_recv.type);
    STRCMP_EQUAL(sdes_item_send_2.text, sdes_item_recv.text);
    CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
    CHECK_FALSE(sdes_it.error());

    CHECK_EQUAL(Traverser::Iterator::XR, it.next());
    XrTraverser xr_tr = it.get_xr();
    CHECK(xr_tr.parse());
    CHECK_EQUAL(2, xr_tr.blocks_count());
    CHECK_EQUAL(111, xr_tr.packet().ssrc());
    XrTraverser::Iterator xr_it = xr_tr.iter();
    CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
    CHECK_EQUAL(ref_time.ntp_timestamp(), xr_it.get_rrtr().ntp_timestamp());
    CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());
    const header::XrDlrrBlock& pdlrr = xr_it.get_dlrr();

    CHECK_EQUAL(2, pdlrr.num_subblocks());
    CHECK_EQUAL(dlrr_repblock_1.ssrc(), pdlrr.get_subblock(0).ssrc());
    CHECK_EQUAL(dlrr_repblock_1.delay_last_rr(), pdlrr.get_subblock(0).delay_last_rr());
    CHECK_EQUAL(dlrr_repblock_1.last_rr(), pdlrr.get_subblock(0).last_rr());
    CHECK_EQUAL(dlrr_repblock_2.ssrc(), pdlrr.get_subblock(1).ssrc());
    CHECK_EQUAL(dlrr_repblock_2.delay_last_rr(), pdlrr.get_subblock(1).delay_last_rr());
    CHECK_EQUAL(dlrr_repblock_2.last_rr(), pdlrr.get_subblock(1).last_rr());
    CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
    CHECK_FALSE(xr_it.error());

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(builder_traverser, rr_xr_padding) {
    core::Slice<uint8_t> buff = new_buffer();

    header::ReceiverReportPacket rr;
    rr.set_ssrc(1);

    header::ReceptionReportBlock receiver_report;
    receiver_report.set_ssrc(1);
    receiver_report.set_fract_loss(1, 8);
    receiver_report.set_cumloss(2);
    receiver_report.set_last_seqnum(3);
    receiver_report.set_jitter(4);
    receiver_report.set_last_sr(5);
    receiver_report.set_delay_last_sr(6);

    header::XrPacket xr;
    xr.set_ssrc(111);
    header::XrRrtrBlock ref_time;
    ref_time.set_ntp_timestamp(0xFFFFFFFFFFFFFFFF);
    header::XrDlrrBlock dlrr;
    header::XrDlrrSubblock dlrr_repblock_1;
    dlrr_repblock_1.set_ssrc(222);
    dlrr_repblock_1.set_delay_last_rr(333);
    dlrr_repblock_1.set_last_rr(444);
    header::XrDlrrSubblock dlrr_repblock_2;
    dlrr_repblock_2.set_ssrc(555);
    dlrr_repblock_2.set_delay_last_rr(666);
    dlrr_repblock_2.set_last_rr(777);

    // Synthesize part

    Builder builder(buff);

    // RR
    builder.begin_rr(rr);
    builder.add_rr_report(receiver_report);
    builder.end_rr();

    // XR
    builder.begin_xr(xr);
    builder.add_xr_rrtr(ref_time);
    builder.begin_xr_dlrr(dlrr);
    builder.add_xr_dlrr_report(dlrr_repblock_1);
    builder.add_xr_dlrr_report(dlrr_repblock_2);
    builder.end_xr_dlrr();
    builder.end_xr();

    // Padding
    builder.add_padding(64);

    // Validation part

    validate_buffer(buff);

    // Parsing part

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::RR, it.next());
    CHECK_EQUAL(rr.ssrc(), it.get_rr().ssrc());

    CHECK_EQUAL(receiver_report.ssrc(), it.get_rr().get_block(0).ssrc());
    DOUBLES_EQUAL(receiver_report.fract_loss(), it.get_rr().get_block(0).fract_loss(),
                  1e-8);
    CHECK_EQUAL(receiver_report.cumloss(), it.get_rr().get_block(0).cumloss());
    CHECK_EQUAL(receiver_report.last_seqnum(), it.get_rr().get_block(0).last_seqnum());
    CHECK_EQUAL(receiver_report.jitter(), it.get_rr().get_block(0).jitter());
    CHECK_EQUAL(receiver_report.last_sr(), it.get_rr().get_block(0).last_sr());
    CHECK_EQUAL(receiver_report.delay_last_sr(),
                it.get_rr().get_block(0).delay_last_sr());

    CHECK_EQUAL(Traverser::Iterator::XR, it.next());
    XrTraverser xr_tr = it.get_xr();
    CHECK(xr_tr.parse());
    CHECK_EQUAL(2, xr_tr.blocks_count());
    CHECK_EQUAL(111, xr_tr.packet().ssrc());
    XrTraverser::Iterator xr_it = xr_tr.iter();
    CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
    CHECK_EQUAL(ref_time.ntp_timestamp(), xr_it.get_rrtr().ntp_timestamp());
    CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());
    const header::XrDlrrBlock& pdlrr = xr_it.get_dlrr();

    CHECK_EQUAL(2, pdlrr.num_subblocks());
    CHECK_EQUAL(dlrr_repblock_1.ssrc(), pdlrr.get_subblock(0).ssrc());
    CHECK_EQUAL(dlrr_repblock_1.delay_last_rr(), pdlrr.get_subblock(0).delay_last_rr());
    CHECK_EQUAL(dlrr_repblock_1.last_rr(), pdlrr.get_subblock(0).last_rr());
    CHECK_EQUAL(dlrr_repblock_2.ssrc(), pdlrr.get_subblock(1).ssrc());
    CHECK_EQUAL(dlrr_repblock_2.delay_last_rr(), pdlrr.get_subblock(1).delay_last_rr());
    CHECK_EQUAL(dlrr_repblock_2.last_rr(), pdlrr.get_subblock(1).last_rr());
    CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
    CHECK_FALSE(xr_it.error());

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(builder_traverser, bye) {
    core::Slice<uint8_t> buff = new_buffer();

    header::ReceiverReportPacket rr;
    rr.set_ssrc(1);

    const char* s_reason = "Reason to live";

    // Synthesize part

    Builder builder(buff);

    // Empty RR (RFC3550 Page 21)
    builder.begin_rr(rr);
    builder.end_rr();

    // BYE
    builder.begin_bye();
    builder.add_bye_ssrc(222);
    builder.add_bye_ssrc(333);
    builder.add_bye_ssrc(444);
    builder.add_bye_ssrc(555);
    builder.add_bye_reason(s_reason);
    builder.end_bye();

    // Validation part

    validate_buffer(buff);

    // Parsing part

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::RR, it.next());
    CHECK_EQUAL(Traverser::Iterator::BYE, it.next());
    ByeTraverser bye_recv = it.get_bye();

    CHECK(bye_recv.parse());
    CHECK_EQUAL(4, bye_recv.ssrc_count());
    ByeTraverser::Iterator bye_it = bye_recv.iter();
    CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
    CHECK_EQUAL(222, bye_it.get_ssrc());
    CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
    CHECK_EQUAL(333, bye_it.get_ssrc());
    CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
    CHECK_EQUAL(444, bye_it.get_ssrc());
    CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
    CHECK_EQUAL(555, bye_it.get_ssrc());
    CHECK_EQUAL(ByeTraverser::Iterator::REASON, bye_it.next());
    STRCMP_EQUAL(s_reason, bye_it.get_reason());
    CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
    CHECK_FALSE(bye_it.error());

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

} // namespace rtcp
} // namespace roc
