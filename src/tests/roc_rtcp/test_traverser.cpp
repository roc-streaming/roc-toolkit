/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtcp/headers.h"
#include "roc_rtcp/traverser.h"

namespace roc {
namespace rtcp {
namespace {

enum { MaxBufSize = 1492 };

core::HeapArena arena;
packet::PacketFactory packet_factory(arena, MaxBufSize);

core::Slice<uint8_t> new_buffer() {
    core::Slice<uint8_t> buff = packet_factory.new_packet_buffer();
    CHECK(buff);
    return buff.subslice(0, 0);
}

void append_buffer(core::Slice<uint8_t>& buff, const void* data, size_t data_sz) {
    memcpy(buff.extend(data_sz), data, data_sz);
}

} // namespace

TEST_GROUP(traverser) {};

TEST(traverser, no_packets) {
    { // empty buffer
        core::Slice<uint8_t> buff = new_buffer();

        Traverser traverser(buff);
        CHECK(!traverser.parse());
    }
    { // zero buffer
        core::Slice<uint8_t> buff = new_buffer();

        char zeros[100] = {};
        append_buffer(buff, &zeros, sizeof(zeros));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // unknown packet type
        core::Slice<uint8_t> buff = new_buffer();

        char hdr_data[100] = {};

        header::PacketHeader* hdr = (header::PacketHeader*)hdr_data;
        hdr_data[1] = 123; // type
        hdr->set_version(header::V2);
        hdr->set_counter(1);
        hdr->set_len_bytes(sizeof(hdr_data));

        append_buffer(buff, &hdr_data, sizeof(hdr_data));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // unknown packet version
        core::Slice<uint8_t> buff = new_buffer();

        char hdr_data[100] = {};

        header::PacketHeader* hdr = (header::PacketHeader*)hdr_data;
        hdr_data[1] = 123; // type
        hdr->set_version((header::Version)3);
        hdr->set_counter(1);
        hdr->set_len_bytes(sizeof(hdr_data));

        append_buffer(buff, &hdr_data, sizeof(hdr_data));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
}

TEST(traverser, sr_iteration) {
    header::SenderReportPacket sr;
    sr.header().set_counter(2);
    sr.header().set_len_bytes(sizeof(header::SenderReportPacket)
                              + sizeof(header::ReceptionReportBlock) * 2);
    sr.set_ssrc(111);

    header::ReceptionReportBlock blk1;
    blk1.set_ssrc(222);
    header::ReceptionReportBlock blk2;
    blk2.set_ssrc(333);

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sr, sizeof(sr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::SR, it.next());
        CHECK_EQUAL(2, it.get_sr().num_blocks());
        CHECK_EQUAL(111, it.get_sr().ssrc());
        CHECK_EQUAL(222, it.get_sr().get_block(0).ssrc());
        CHECK_EQUAL(333, it.get_sr().get_block(1).ssrc());

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // truncated buffer (header)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sr, sizeof(sr) - 1);

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated buffer (block)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sr, sizeof(sr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2) - 1);

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (header)
        core::Slice<uint8_t> buff = new_buffer();

        header::SenderReportPacket sr_copy = sr;
        sr_copy.header().set_counter(0);
        sr_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::SenderReportPacket)) - 1);

        append_buffer(buff, &sr_copy, sizeof(sr_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (block)
        core::Slice<uint8_t> buff = new_buffer();

        header::SenderReportPacket sr_copy = sr;
        sr_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::SenderReportPacket)
                                         + sizeof(header::ReceptionReportBlock) * 2)
            - 1);

        append_buffer(buff, &sr_copy, sizeof(sr_copy));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // smaller block counter
        core::Slice<uint8_t> buff = new_buffer();

        header::SenderReportPacket sr_copy = sr;
        sr_copy.header().set_counter(1);

        append_buffer(buff, &sr_copy, sizeof(sr_copy));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::SR, it.next());
        CHECK_EQUAL(1, it.get_sr().num_blocks());
        CHECK_EQUAL(111, it.get_sr().ssrc());
        CHECK_EQUAL(222, it.get_sr().get_block(0).ssrc());

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // larger block counter
        core::Slice<uint8_t> buff = new_buffer();

        header::SenderReportPacket sr_copy = sr;
        sr_copy.header().set_counter(3);

        append_buffer(buff, &sr_copy, sizeof(sr_copy));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // zero blocks
        core::Slice<uint8_t> buff = new_buffer();

        header::SenderReportPacket sr_copy = sr;
        sr_copy.header().set_counter(0);
        sr_copy.header().set_len_bytes(sizeof(header::SenderReportPacket));

        append_buffer(buff, &sr_copy, sizeof(sr_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::SR, it.next());
        CHECK_EQUAL(0, it.get_sr().num_blocks());
        CHECK_EQUAL(111, it.get_sr().ssrc());

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
}

TEST(traverser, sr_padding) {
    char packet_padding[16] = {};
    packet_padding[15] = 16;

    header::SenderReportPacket sr;
    sr.header().set_padding(true);
    sr.header().set_counter(2);
    sr.header().set_len_bytes(sizeof(header::SenderReportPacket)
                              + sizeof(header::ReceptionReportBlock) * 2
                              + sizeof(packet_padding));
    sr.set_ssrc(111);

    header::ReceptionReportBlock blk1;
    blk1.set_ssrc(222);
    header::ReceptionReportBlock blk2;
    blk2.set_ssrc(333);

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sr, sizeof(sr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));
        append_buffer(buff, &packet_padding, sizeof(packet_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::SR, it.next());
        CHECK_EQUAL(2, it.get_sr().num_blocks());
        CHECK_EQUAL(111, it.get_sr().ssrc());
        CHECK_EQUAL(222, it.get_sr().get_block(0).ssrc());
        CHECK_EQUAL(333, it.get_sr().get_block(1).ssrc());

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // padding length is zero
        char bad_padding[16] = {};
        bad_padding[15] = 0;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sr, sizeof(sr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // padding length is too big
        char bad_padding[16] = {};
        bad_padding[15] = 127;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sr, sizeof(sr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
}

TEST(traverser, sr_fields) {
    core::Slice<uint8_t> buff = new_buffer();

    {
        header::SenderReportPacket sr;
        sr.header().set_counter(2);
        sr.header().set_len_bytes(sizeof(header::SenderReportPacket)
                                  + sizeof(header::ReceptionReportBlock) * 2);
        sr.set_ssrc(100);
        sr.set_ntp_timestamp(101);
        sr.set_rtp_timestamp(102);
        sr.set_packet_count(103);
        sr.set_byte_count(104);

        header::ReceptionReportBlock blk1;
        blk1.set_ssrc(10);
        blk1.set_fract_loss(0.5f);
        blk1.set_cum_loss(13);
        blk1.set_last_seqnum(14);
        blk1.set_jitter(15);
        blk1.set_last_sr(0x100000);
        blk1.set_delay_last_sr(0x200000);

        header::ReceptionReportBlock blk2;
        blk2.set_ssrc(20);
        blk2.set_fract_loss(0.75f);
        blk2.set_cum_loss(23);
        blk2.set_last_seqnum(24);
        blk2.set_jitter(25);
        blk2.set_last_sr(0x300000);
        blk2.set_delay_last_sr(0x400000);

        append_buffer(buff, &sr, sizeof(sr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));
    }

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::SR, it.next());

    CHECK_EQUAL(100, it.get_sr().ssrc());
    CHECK_EQUAL(101, it.get_sr().ntp_timestamp());
    CHECK_EQUAL(102, it.get_sr().rtp_timestamp());
    CHECK_EQUAL(103, it.get_sr().packet_count());
    CHECK_EQUAL(104, it.get_sr().byte_count());

    CHECK_EQUAL(2, it.get_sr().num_blocks());

    CHECK_EQUAL(10, it.get_sr().get_block(0).ssrc());
    DOUBLES_EQUAL(0.5, it.get_sr().get_block(0).fract_loss(), 1e-8);
    CHECK_EQUAL(13, it.get_sr().get_block(0).cum_loss());
    CHECK_EQUAL(14, it.get_sr().get_block(0).last_seqnum());
    CHECK_EQUAL(15, it.get_sr().get_block(0).jitter());
    CHECK_EQUAL(0x100000, it.get_sr().get_block(0).last_sr());
    CHECK_EQUAL(0x200000, it.get_sr().get_block(0).delay_last_sr());

    CHECK_EQUAL(20, it.get_sr().get_block(1).ssrc());
    DOUBLES_EQUAL(0.75, it.get_sr().get_block(1).fract_loss(), 1e-8);
    CHECK_EQUAL(23, it.get_sr().get_block(1).cum_loss());
    CHECK_EQUAL(24, it.get_sr().get_block(1).last_seqnum());
    CHECK_EQUAL(25, it.get_sr().get_block(1).jitter());
    CHECK_EQUAL(0x300000, it.get_sr().get_block(1).last_sr());
    CHECK_EQUAL(0x400000, it.get_sr().get_block(1).delay_last_sr());

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(traverser, rr_iteration) {
    header::ReceiverReportPacket rr;
    rr.header().set_counter(2);
    rr.header().set_len_bytes(sizeof(header::ReceiverReportPacket)
                              + sizeof(header::ReceptionReportBlock) * 2);
    rr.set_ssrc(111);

    header::ReceptionReportBlock blk1;
    blk1.set_ssrc(222);
    header::ReceptionReportBlock blk2;
    blk2.set_ssrc(333);

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &rr, sizeof(rr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::RR, it.next());
        CHECK_EQUAL(2, it.get_rr().num_blocks());
        CHECK_EQUAL(111, it.get_rr().ssrc());
        CHECK_EQUAL(222, it.get_rr().get_block(0).ssrc());
        CHECK_EQUAL(333, it.get_rr().get_block(1).ssrc());

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // truncated buffer (header)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &rr, sizeof(rr) - 1);

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated buffer (block)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &rr, sizeof(rr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2) - 1);

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (header)
        core::Slice<uint8_t> buff = new_buffer();

        header::ReceiverReportPacket rr_copy = rr;
        rr_copy.header().set_counter(0);
        rr_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::ReceiverReportPacket)) - 1);

        append_buffer(buff, &rr_copy, sizeof(rr_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (block)
        core::Slice<uint8_t> buff = new_buffer();

        header::ReceiverReportPacket rr_copy = rr;
        rr_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::ReceiverReportPacket)
                                         + sizeof(header::ReceptionReportBlock) * 2)
            - 1);

        append_buffer(buff, &rr_copy, sizeof(rr_copy));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // smaller block counter
        core::Slice<uint8_t> buff = new_buffer();

        header::ReceiverReportPacket rr_copy = rr;
        rr_copy.header().set_counter(1);

        append_buffer(buff, &rr_copy, sizeof(rr_copy));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::RR, it.next());
        CHECK_EQUAL(1, it.get_rr().num_blocks());
        CHECK_EQUAL(111, it.get_rr().ssrc());
        CHECK_EQUAL(222, it.get_rr().get_block(0).ssrc());

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // larger block counter
        core::Slice<uint8_t> buff = new_buffer();

        header::ReceiverReportPacket rr_copy = rr;
        rr_copy.header().set_counter(3);

        append_buffer(buff, &rr_copy, sizeof(rr_copy));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // zero blocks
        core::Slice<uint8_t> buff = new_buffer();

        header::ReceiverReportPacket rr_copy = rr;
        rr_copy.header().set_counter(0);
        rr_copy.header().set_len_bytes(sizeof(header::ReceiverReportPacket));

        append_buffer(buff, &rr_copy, sizeof(rr_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::RR, it.next());
        CHECK_EQUAL(0, it.get_rr().num_blocks());
        CHECK_EQUAL(111, it.get_rr().ssrc());

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
}

TEST(traverser, rr_padding) {
    char packet_padding[16] = {};
    packet_padding[15] = 16;

    header::ReceiverReportPacket rr;
    rr.header().set_padding(true);
    rr.header().set_counter(2);
    rr.header().set_len_bytes(sizeof(header::ReceiverReportPacket)
                              + sizeof(header::ReceptionReportBlock) * 2
                              + sizeof(packet_padding));
    rr.set_ssrc(111);

    header::ReceptionReportBlock blk1;
    blk1.set_ssrc(222);
    header::ReceptionReportBlock blk2;
    blk2.set_ssrc(333);

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &rr, sizeof(rr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));
        append_buffer(buff, &packet_padding, sizeof(packet_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::RR, it.next());
        CHECK_EQUAL(2, it.get_rr().num_blocks());
        CHECK_EQUAL(111, it.get_rr().ssrc());
        CHECK_EQUAL(222, it.get_rr().get_block(0).ssrc());
        CHECK_EQUAL(333, it.get_rr().get_block(1).ssrc());

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // padding length is zero
        char bad_padding[16] = {};
        bad_padding[15] = 0;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &rr, sizeof(rr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // padding length is too big
        char bad_padding[16] = {};
        bad_padding[15] = 127;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &rr, sizeof(rr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
}

TEST(traverser, rr_fields) {
    core::Slice<uint8_t> buff = new_buffer();

    {
        header::ReceiverReportPacket rr;
        rr.header().set_counter(2);
        rr.header().set_len_bytes(sizeof(header::ReceiverReportPacket)
                                  + sizeof(header::ReceptionReportBlock) * 2);
        rr.set_ssrc(100);

        header::ReceptionReportBlock blk1;
        blk1.set_ssrc(10);
        blk1.set_fract_loss(0.5f);
        blk1.set_cum_loss(13);
        blk1.set_last_seqnum(14);
        blk1.set_jitter(15);
        blk1.set_last_sr(0x100000);
        blk1.set_delay_last_sr(0x200000);

        header::ReceptionReportBlock blk2;
        blk2.set_ssrc(20);
        blk2.set_fract_loss(0.75f);
        blk2.set_cum_loss(23);
        blk2.set_last_seqnum(24);
        blk2.set_jitter(25);
        blk2.set_last_sr(0x300000);
        blk2.set_delay_last_sr(0x400000);

        append_buffer(buff, &rr, sizeof(rr));
        append_buffer(buff, &blk1, sizeof(blk1));
        append_buffer(buff, &blk2, sizeof(blk2));
    }

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::RR, it.next());

    CHECK_EQUAL(100, it.get_rr().ssrc());

    CHECK_EQUAL(2, it.get_rr().num_blocks());

    CHECK_EQUAL(10, it.get_rr().get_block(0).ssrc());
    DOUBLES_EQUAL(0.5, it.get_rr().get_block(0).fract_loss(), 1e-8);
    CHECK_EQUAL(13, it.get_rr().get_block(0).cum_loss());
    CHECK_EQUAL(14, it.get_rr().get_block(0).last_seqnum());
    CHECK_EQUAL(15, it.get_rr().get_block(0).jitter());
    CHECK_EQUAL(0x100000, it.get_rr().get_block(0).last_sr());
    CHECK_EQUAL(0x200000, it.get_rr().get_block(0).delay_last_sr());

    CHECK_EQUAL(20, it.get_rr().get_block(1).ssrc());
    DOUBLES_EQUAL(0.75, it.get_rr().get_block(1).fract_loss(), 1e-8);
    CHECK_EQUAL(23, it.get_rr().get_block(1).cum_loss());
    CHECK_EQUAL(24, it.get_rr().get_block(1).last_seqnum());
    CHECK_EQUAL(25, it.get_rr().get_block(1).jitter());
    CHECK_EQUAL(0x300000, it.get_rr().get_block(1).last_sr());
    CHECK_EQUAL(0x400000, it.get_rr().get_block(1).delay_last_sr());

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(traverser, xr_iteration) {
    header::XrPacket xr;
    xr.header().set_len_bytes(sizeof(header::XrPacket) + sizeof(header::XrRrtrBlock)
                              + sizeof(header::XrDlrrBlock)
                              + sizeof(header::XrDlrrSubblock) * 2);
    xr.set_ssrc(111);

    header::XrRrtrBlock rrtr;
    rrtr.header().set_len_bytes(sizeof(header::XrRrtrBlock));
    rrtr.header().set_type_specific(22);

    header::XrDlrrBlock dlrr;
    dlrr.header().set_len_bytes(sizeof(header::XrDlrrBlock)
                                + sizeof(header::XrDlrrSubblock) * 2);
    dlrr.header().set_type_specific(33);

    header::XrDlrrSubblock dlrr_sblk1;
    dlrr_sblk1.set_ssrc(444);
    header::XrDlrrSubblock dlrr_sblk2;
    dlrr_sblk2.set_ssrc(555);

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &rrtr, sizeof(rrtr));
        append_buffer(buff, &dlrr, sizeof(dlrr));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(xr_tr.parse());

            CHECK_EQUAL(2, xr_tr.blocks_count());
            CHECK_EQUAL(111, xr_tr.packet().ssrc());

            XrTraverser::Iterator xr_it = xr_tr.iter();
            CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
            CHECK_EQUAL(22, xr_it.get_rrtr().header().type_specific());
            CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());
            CHECK_EQUAL(2, xr_it.get_dlrr().num_subblocks());
            CHECK_EQUAL(33, xr_it.get_dlrr().header().type_specific());
            CHECK_EQUAL(444, xr_it.get_dlrr().get_subblock(0).ssrc());
            CHECK_EQUAL(555, xr_it.get_dlrr().get_subblock(1).ssrc());
            CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
            CHECK_FALSE(xr_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // truncated buffer (header)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &xr, sizeof(xr) - 1);

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated buffer (block)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &rrtr, sizeof(rrtr));
        append_buffer(buff, &dlrr, sizeof(dlrr));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2) - 1);

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (header)
        core::Slice<uint8_t> buff = new_buffer();

        header::XrPacket xr_copy = xr;
        xr_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::XrPacket)) - 1);

        append_buffer(buff, &xr_copy, sizeof(xr_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(!xr_tr.parse());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (rrtr)
        core::Slice<uint8_t> buff = new_buffer();

        header::XrRrtrBlock rrtr_copy = rrtr;
        rrtr_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::XrRrtrBlock)) - 1);

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &rrtr_copy, sizeof(rrtr_copy));
        append_buffer(buff, &dlrr, sizeof(dlrr));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(xr_tr.parse());

            CHECK_EQUAL(1, xr_tr.blocks_count());
            CHECK_EQUAL(111, xr_tr.packet().ssrc());

            XrTraverser::Iterator xr_it = xr_tr.iter();
            CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());
            CHECK_EQUAL(2, xr_it.get_dlrr().num_subblocks());
            CHECK_EQUAL(33, xr_it.get_dlrr().header().type_specific());
            CHECK_EQUAL(444, xr_it.get_dlrr().get_subblock(0).ssrc());
            CHECK_EQUAL(555, xr_it.get_dlrr().get_subblock(1).ssrc());
            CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
            CHECK_TRUE(xr_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // truncated len (dlrr)
        core::Slice<uint8_t> buff = new_buffer();

        header::XrDlrrBlock dlrr_copy = dlrr;
        dlrr_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::XrDlrrBlock)) - 1);

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &rrtr, sizeof(rrtr));
        append_buffer(buff, &dlrr_copy, sizeof(dlrr_copy));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(xr_tr.parse());

            CHECK_EQUAL(1, xr_tr.blocks_count());
            CHECK_EQUAL(111, xr_tr.packet().ssrc());

            XrTraverser::Iterator xr_it = xr_tr.iter();
            CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
            CHECK_EQUAL(22, xr_it.get_rrtr().header().type_specific());
            CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
            CHECK_TRUE(xr_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // truncated len (dlrr subblock)
        core::Slice<uint8_t> buff = new_buffer();

        header::XrDlrrBlock dlrr_copy = dlrr;
        dlrr_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::XrDlrrBlock)
                                         + sizeof(header::XrDlrrSubblock) * 2)
            - 1);

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &rrtr, sizeof(rrtr));
        append_buffer(buff, &dlrr_copy, sizeof(dlrr_copy));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(xr_tr.parse());

            CHECK_EQUAL(2, xr_tr.blocks_count());
            CHECK_EQUAL(111, xr_tr.packet().ssrc());

            XrTraverser::Iterator xr_it = xr_tr.iter();
            CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
            CHECK_EQUAL(22, xr_it.get_rrtr().header().type_specific());
            CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());
            CHECK_EQUAL(1, xr_it.get_dlrr().num_subblocks());
            CHECK_EQUAL(33, xr_it.get_dlrr().header().type_specific());
            CHECK_EQUAL(444, xr_it.get_dlrr().get_subblock(0).ssrc());
            CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
            CHECK_FALSE(xr_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // zero blocks
        core::Slice<uint8_t> buff = new_buffer();

        header::XrPacket xr_copy = xr;
        xr_copy.header().set_len_bytes(sizeof(header::XrPacket));

        append_buffer(buff, &xr_copy, sizeof(xr_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(xr_tr.parse());

            CHECK_EQUAL(0, xr_tr.blocks_count());
            CHECK_EQUAL(111, xr_tr.packet().ssrc());

            XrTraverser::Iterator xr_it = xr_tr.iter();
            CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
            CHECK_FALSE(xr_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // zero subblocks
        core::Slice<uint8_t> buff = new_buffer();

        header::XrPacket xr_copy = xr;
        xr_copy.header().set_len_bytes(sizeof(header::XrPacket)
                                       + sizeof(header::XrRrtrBlock)
                                       + sizeof(header::XrDlrrBlock));

        header::XrDlrrBlock dlrr_copy = dlrr;
        dlrr_copy.header().set_len_bytes(sizeof(header::XrDlrrBlock));

        append_buffer(buff, &xr_copy, sizeof(xr_copy));
        append_buffer(buff, &rrtr, sizeof(rrtr));
        append_buffer(buff, &dlrr_copy, sizeof(dlrr_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(xr_tr.parse());

            CHECK_EQUAL(2, xr_tr.blocks_count());
            CHECK_EQUAL(111, xr_tr.packet().ssrc());

            XrTraverser::Iterator xr_it = xr_tr.iter();
            CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
            CHECK_EQUAL(22, xr_it.get_rrtr().header().type_specific());
            CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());
            CHECK_EQUAL(0, xr_it.get_dlrr().num_subblocks());
            CHECK_EQUAL(33, xr_it.get_dlrr().header().type_specific());
            CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
            CHECK_FALSE(xr_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // unknown block type
        core::Slice<uint8_t> buff = new_buffer();

        header::XrPacket xr_copy = xr;
        xr_copy.header().set_len_bytes(sizeof(header::XrPacket)
                                       + sizeof(header::XrBlockHeader)
                                       + sizeof(header::XrRrtrBlock));

        char blk_data[sizeof(header::XrBlockHeader)] = {};

        header::XrBlockHeader* blk = (header::XrBlockHeader*)blk_data;
        blk_data[0] = 123; // type
        blk->set_len_bytes(sizeof(blk_data));

        append_buffer(buff, &xr_copy, sizeof(xr_copy));
        append_buffer(buff, blk_data, sizeof(blk_data));
        append_buffer(buff, &rrtr, sizeof(rrtr));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(xr_tr.parse());

            CHECK_EQUAL(1, xr_tr.blocks_count());
            CHECK_EQUAL(111, xr_tr.packet().ssrc());

            XrTraverser::Iterator xr_it = xr_tr.iter();
            CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
            CHECK_EQUAL(22, xr_it.get_rrtr().header().type_specific());
            CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
            CHECK_FALSE(xr_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
}

TEST(traverser, xr_padding) {
    char packet_padding[16] = {};
    packet_padding[15] = 16;

    header::XrPacket xr;
    xr.header().set_padding(true);
    xr.header().set_len_bytes(sizeof(header::XrPacket) + sizeof(header::XrDlrrBlock)
                              + sizeof(header::XrDlrrSubblock) * 2
                              + sizeof(packet_padding));
    xr.set_ssrc(111);

    header::XrDlrrBlock dlrr;
    dlrr.header().set_len_bytes(sizeof(header::XrDlrrBlock)
                                + sizeof(header::XrDlrrSubblock) * 2);

    header::XrDlrrSubblock dlrr_sblk1;
    dlrr_sblk1.set_ssrc(222);
    header::XrDlrrSubblock dlrr_sblk2;
    dlrr_sblk2.set_ssrc(333);

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &dlrr, sizeof(dlrr));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2));
        append_buffer(buff, &packet_padding, sizeof(packet_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(xr_tr.parse());

            CHECK_EQUAL(1, xr_tr.blocks_count());
            CHECK_EQUAL(111, xr_tr.packet().ssrc());

            XrTraverser::Iterator xr_it = xr_tr.iter();
            CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());
            CHECK_EQUAL(2, xr_it.get_dlrr().num_subblocks());
            CHECK_EQUAL(222, xr_it.get_dlrr().get_subblock(0).ssrc());
            CHECK_EQUAL(333, xr_it.get_dlrr().get_subblock(1).ssrc());
            CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
            CHECK_FALSE(xr_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // padding length is zero
        char bad_padding[16] = {};
        bad_padding[15] = 0;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &dlrr, sizeof(dlrr));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(!xr_tr.parse());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // padding length is too big
        char bad_padding[16] = {};
        bad_padding[15] = 127;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &dlrr, sizeof(dlrr));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();

        CHECK_EQUAL(Traverser::Iterator::XR, it.next());

        {
            XrTraverser xr_tr = it.get_xr();
            CHECK(!xr_tr.parse());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
}

TEST(traverser, xr_fields) {
    core::Slice<uint8_t> buff = new_buffer();

    {
        header::XrPacket xr;
        xr.header().set_len_bytes(
            sizeof(header::XrPacket) + sizeof(header::XrRrtrBlock)
            + sizeof(header::XrDlrrBlock) + sizeof(header::XrDlrrSubblock) * 2
            + sizeof(header::XrMeasurementInfoBlock) + sizeof(header::XrDelayMetricsBlock)
            + sizeof(header::XrQueueMetricsBlock));
        xr.set_ssrc(111);

        header::XrRrtrBlock rrtr;
        rrtr.header().set_len_bytes(sizeof(header::XrRrtrBlock));
        rrtr.set_ntp_timestamp(123456789);

        header::XrDlrrBlock dlrr;
        dlrr.header().set_len_bytes(sizeof(header::XrDlrrBlock)
                                    + sizeof(header::XrDlrrSubblock) * 2);

        header::XrDlrrSubblock dlrr_sblk1;
        dlrr_sblk1.set_ssrc(222);
        dlrr_sblk1.set_last_rr(0x100000);
        dlrr_sblk1.set_delay_last_rr(0x200000);

        header::XrDlrrSubblock dlrr_sblk2;
        dlrr_sblk2.set_ssrc(333);
        dlrr_sblk2.set_last_rr(0x300000);
        dlrr_sblk2.set_delay_last_rr(0x400000);

        header::XrMeasurementInfoBlock measure_info;
        measure_info.header().set_len_bytes(sizeof(header::XrMeasurementInfoBlock));
        measure_info.set_ssrc(444);
        measure_info.set_first_seq(41);
        measure_info.set_interval_first_seq(42);
        measure_info.set_interval_last_seq(43);
        measure_info.set_interval_duration(0x500000);
        measure_info.set_cum_duration(0x6000000000000006);

        header::XrDelayMetricsBlock delay_metrics;
        delay_metrics.header().set_len_bytes(sizeof(header::XrDelayMetricsBlock));
        delay_metrics.set_metric_flag(header::MetricFlag_IntervalDuration);
        delay_metrics.set_ssrc(555);
        delay_metrics.set_mean_rtt(0x600000);
        delay_metrics.set_min_rtt(0x700000);
        delay_metrics.set_max_rtt(0x800000);
        delay_metrics.set_e2e_latency(0x9000000000000009);

        header::XrQueueMetricsBlock queue_metrics;
        queue_metrics.header().set_len_bytes(sizeof(header::XrQueueMetricsBlock));
        queue_metrics.set_metric_flag(header::MetricFlag_SampledValue);
        queue_metrics.set_ssrc(666);
        queue_metrics.set_niq_latency(0xA00000);
        queue_metrics.set_niq_stalling(0xB00000);

        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &rrtr, sizeof(rrtr));
        append_buffer(buff, &dlrr, sizeof(dlrr));
        append_buffer(buff, &dlrr_sblk1, sizeof(dlrr_sblk1));
        append_buffer(buff, &dlrr_sblk2, sizeof(dlrr_sblk2));
        append_buffer(buff, &measure_info, sizeof(measure_info));
        append_buffer(buff, &delay_metrics, sizeof(delay_metrics));
        append_buffer(buff, &queue_metrics, sizeof(queue_metrics));
    }

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::XR, it.next());

    {
        XrTraverser xr_tr = it.get_xr();
        CHECK(xr_tr.parse());

        CHECK_EQUAL(5, xr_tr.blocks_count());
        CHECK_EQUAL(111, xr_tr.packet().ssrc());

        XrTraverser::Iterator xr_it = xr_tr.iter();

        CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
        CHECK_EQUAL(123456789, xr_it.get_rrtr().ntp_timestamp());

        CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());

        CHECK_EQUAL(2, xr_it.get_dlrr().num_subblocks());

        CHECK_EQUAL(222, xr_it.get_dlrr().get_subblock(0).ssrc());
        CHECK_EQUAL(0x100000, xr_it.get_dlrr().get_subblock(0).last_rr());
        CHECK_EQUAL(0x200000, xr_it.get_dlrr().get_subblock(0).delay_last_rr());

        CHECK_EQUAL(333, xr_it.get_dlrr().get_subblock(1).ssrc());
        CHECK_EQUAL(0x300000, xr_it.get_dlrr().get_subblock(1).last_rr());
        CHECK_EQUAL(0x400000, xr_it.get_dlrr().get_subblock(1).delay_last_rr());

        CHECK_EQUAL(XrTraverser::Iterator::MEASUREMENT_INFO_BLOCK, xr_it.next());
        CHECK_EQUAL(444, xr_it.get_measurement_info().ssrc());
        CHECK_EQUAL(41, xr_it.get_measurement_info().first_seq());
        CHECK_EQUAL(42, xr_it.get_measurement_info().interval_first_seq());
        CHECK_EQUAL(43, xr_it.get_measurement_info().interval_last_seq());
        CHECK_EQUAL(0x500000, xr_it.get_measurement_info().interval_duration());
        CHECK_EQUAL(0x6000000000000006, xr_it.get_measurement_info().cum_duration());

        CHECK_EQUAL(XrTraverser::Iterator::DELAY_METRICS_BLOCK, xr_it.next());
        CHECK_EQUAL(header::MetricFlag_IntervalDuration,
                    xr_it.get_delay_metrics().metric_flag());
        CHECK_EQUAL(555, xr_it.get_delay_metrics().ssrc());
        CHECK_EQUAL(0x600000, xr_it.get_delay_metrics().mean_rtt());
        CHECK_EQUAL(0x700000, xr_it.get_delay_metrics().min_rtt());
        CHECK_EQUAL(0x800000, xr_it.get_delay_metrics().max_rtt());
        CHECK_EQUAL(0x9000000000000009, xr_it.get_delay_metrics().e2e_latency());

        CHECK_EQUAL(XrTraverser::Iterator::QUEUE_METRICS_BLOCK, xr_it.next());
        CHECK_EQUAL(header::MetricFlag_SampledValue,
                    xr_it.get_queue_metrics().metric_flag());
        CHECK_EQUAL(666, xr_it.get_queue_metrics().ssrc());
        CHECK_EQUAL(0xA00000, xr_it.get_queue_metrics().niq_latency());
        CHECK_EQUAL(0xB00000, xr_it.get_queue_metrics().niq_stalling());

        CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
        CHECK_FALSE(xr_it.error());
    }

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(traverser, sdes_iteration) {
    const char chunk1_cname[4] = { 'a', 'b', 'c', 'd' };
    const char chunk1_padding[2] = { '\0', 'x' };
    const char chunk2_cname[10] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };
    const char chunk2_padding[4] = { '\0', 'x', 'x', 'x' };

    header::SdesPacket sdes;
    sdes.header().set_counter(2);
    sdes.header().set_len_bytes(sizeof(header::SdesPacket)
                                // chunk 1
                                + sizeof(header::SdesChunkHeader)
                                + sizeof(header::SdesItemHeader) + sizeof(chunk1_cname)
                                + sizeof(chunk1_padding)
                                // chunk 2
                                + sizeof(header::SdesChunkHeader)
                                + sizeof(header::SdesItemHeader) + sizeof(chunk2_cname)
                                + sizeof(chunk2_padding));

    header::SdesChunkHeader chunk1;
    chunk1.set_ssrc(111);

    header::SdesItemHeader chunk1_item;
    chunk1_item.set_type(header::SDES_CNAME);
    chunk1_item.set_text_len(sizeof(chunk1_cname));

    header::SdesChunkHeader chunk2;
    chunk2.set_ssrc(222);

    header::SdesItemHeader chunk2_item;
    chunk2_item.set_type(header::SDES_CNAME);
    chunk2_item.set_text_len(sizeof(chunk2_cname));

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sdes, sizeof(sdes));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &chunk1_item, sizeof(chunk1_item));
        append_buffer(buff, &chunk1_cname, sizeof(chunk1_cname));
        append_buffer(buff, &chunk1_padding, sizeof(chunk1_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(2, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();

            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("abcd", sdes_it.get_item().text);
            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(222, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("0123456789", sdes_it.get_item().text);

            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_FALSE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // truncated buffer (header)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sdes, sizeof(sdes) - 1);

        Traverser traverser(buff);
        CHECK(!traverser.parse());
    }
    { // truncated buffer (body)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sdes, sizeof(sdes));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &chunk1_item, sizeof(chunk1_item));
        append_buffer(buff, &chunk1_cname, sizeof(chunk1_cname));
        append_buffer(buff, &chunk1_padding, sizeof(chunk1_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding) - 1);

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (header)
        core::Slice<uint8_t> buff = new_buffer();

        header::SdesPacket sdes_copy = sdes;
        sdes_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::SdesPacket)) - 1);

        append_buffer(buff, &sdes_copy, sizeof(sdes_copy));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &chunk1_item, sizeof(chunk1_item));
        append_buffer(buff, &chunk1_cname, sizeof(chunk1_cname));
        append_buffer(buff, &chunk1_padding, sizeof(chunk1_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (text)
        core::Slice<uint8_t> buff = new_buffer();

        header::SdesPacket sdes_copy = sdes;
        sdes_copy.header().set_len_words(sdes.header().len_words() - 2);

        append_buffer(buff, &sdes_copy, sizeof(sdes_copy));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &chunk1_item, sizeof(chunk1_item));
        append_buffer(buff, &chunk1_cname, sizeof(chunk1_cname));
        append_buffer(buff, &chunk1_padding, sizeof(chunk1_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(2, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();

            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("abcd", sdes_it.get_item().text);
            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(222, sdes_it.get_chunk().ssrc);

            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_TRUE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (padding)
        core::Slice<uint8_t> buff = new_buffer();

        header::SdesPacket sdes_copy = sdes;
        sdes_copy.header().set_len_words(sdes.header().len_words() - 1);

        append_buffer(buff, &sdes_copy, sizeof(sdes_copy));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &chunk1_item, sizeof(chunk1_item));
        append_buffer(buff, &chunk1_cname, sizeof(chunk1_cname));
        append_buffer(buff, &chunk1_padding, sizeof(chunk1_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(2, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();

            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("abcd", sdes_it.get_item().text);
            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(222, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("0123456789", sdes_it.get_item().text);

            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_FALSE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // smaller chunk counter
        core::Slice<uint8_t> buff = new_buffer();

        header::SdesPacket sdes_copy = sdes;
        sdes_copy.header().set_counter(1);

        append_buffer(buff, &sdes_copy, sizeof(sdes_copy));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &chunk1_item, sizeof(chunk1_item));
        append_buffer(buff, &chunk1_cname, sizeof(chunk1_cname));
        append_buffer(buff, &chunk1_padding, sizeof(chunk1_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(1, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();

            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("abcd", sdes_it.get_item().text);

            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_FALSE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // larger chunk counter
        core::Slice<uint8_t> buff = new_buffer();

        header::SdesPacket sdes_copy = sdes;
        sdes_copy.header().set_counter(3);

        append_buffer(buff, &sdes_copy, sizeof(sdes_copy));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &chunk1_item, sizeof(chunk1_item));
        append_buffer(buff, &chunk1_cname, sizeof(chunk1_cname));
        append_buffer(buff, &chunk1_padding, sizeof(chunk1_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(3, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();

            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("abcd", sdes_it.get_item().text);
            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(222, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("0123456789", sdes_it.get_item().text);

            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_TRUE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // zero chunks
        core::Slice<uint8_t> buff = new_buffer();

        header::SdesPacket sdes_copy = sdes;
        sdes_copy.header().set_counter(0);
        sdes_copy.header().set_len_bytes(sizeof(header::SdesPacket));

        append_buffer(buff, &sdes_copy, sizeof(sdes_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(0, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();
            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_FALSE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // zero items
        core::Slice<uint8_t> buff = new_buffer();

        char zero_padding[4] = {};

        header::SdesPacket sdes_copy = sdes;
        sdes_copy.header().set_len_bytes(sizeof(header::SdesPacket)
                                         // chunk 1
                                         + sizeof(header::SdesChunkHeader)
                                         + sizeof(zero_padding)
                                         // chunk 2
                                         + sizeof(header::SdesChunkHeader)
                                         + sizeof(header::SdesItemHeader)
                                         + sizeof(chunk2_cname) + sizeof(chunk2_padding));

        append_buffer(buff, &sdes_copy, sizeof(sdes_copy));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &zero_padding, sizeof(zero_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(2, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();

            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(222, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("0123456789", sdes_it.get_item().text);

            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_FALSE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // zero-length text
        core::Slice<uint8_t> buff = new_buffer();

        header::SdesItemHeader zero_item = chunk1_item;
        zero_item.set_text_len(0);

        char zero_padding[2] = {};

        header::SdesPacket sdes_copy = sdes;
        sdes_copy.header().set_len_bytes(
            sizeof(header::SdesPacket)
            // chunk 1
            + sizeof(header::SdesChunkHeader) + sizeof(header::SdesItemHeader)
            + sizeof(zero_padding)
            // chunk 2
            + sizeof(header::SdesChunkHeader) + sizeof(header::SdesItemHeader)
            + sizeof(chunk2_cname) + sizeof(chunk2_padding));

        append_buffer(buff, &sdes_copy, sizeof(sdes_copy));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &zero_item, sizeof(zero_item));
        append_buffer(buff, &zero_padding, sizeof(zero_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(2, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();

            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("", sdes_it.get_item().text);
            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(222, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("0123456789", sdes_it.get_item().text);

            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_FALSE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
}

TEST(traverser, sdes_padding) {
    char packet_padding[16] = {};
    packet_padding[15] = 16;

    const char cname[4] = { 'a', 'b', 'c', 'd' };
    const char cname_padding[2] = { '\0', 'x' };

    header::SdesPacket sdes;
    sdes.header().set_padding(true);
    sdes.header().set_counter(1);
    sdes.header().set_len_bytes(sizeof(header::SdesPacket)
                                + sizeof(header::SdesChunkHeader)
                                + sizeof(header::SdesItemHeader) + sizeof(cname)
                                + sizeof(cname_padding) + sizeof(packet_padding));

    header::SdesChunkHeader sdes_chunk;
    sdes_chunk.set_ssrc(111);

    header::SdesItemHeader sdes_item;
    sdes_item.set_type(header::SDES_CNAME);
    sdes_item.set_text_len(sizeof(cname));

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sdes, sizeof(sdes));
        append_buffer(buff, &sdes_chunk, sizeof(sdes_chunk));
        append_buffer(buff, &sdes_item, sizeof(sdes_item));
        append_buffer(buff, &cname, sizeof(cname));
        append_buffer(buff, &cname_padding, sizeof(cname_padding));
        append_buffer(buff, &packet_padding, sizeof(packet_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(sdes_tr.parse());

            CHECK_EQUAL(1, sdes_tr.chunks_count());

            SdesTraverser::Iterator sdes_it = sdes_tr.iter();

            CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
            CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
            CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
            STRCMP_EQUAL("abcd", sdes_it.get_item().text);

            CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
            CHECK_FALSE(sdes_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // padding length is zero
        char bad_padding[16] = {};
        bad_padding[15] = 0;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sdes, sizeof(sdes));
        append_buffer(buff, &sdes_chunk, sizeof(sdes_chunk));
        append_buffer(buff, &sdes_item, sizeof(sdes_item));
        append_buffer(buff, &cname, sizeof(cname));
        append_buffer(buff, &cname_padding, sizeof(cname_padding));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(!sdes_tr.parse());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // padding length is too big
        char bad_padding[16] = {};
        bad_padding[15] = 127;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &sdes, sizeof(sdes));
        append_buffer(buff, &sdes_chunk, sizeof(sdes_chunk));
        append_buffer(buff, &sdes_item, sizeof(sdes_item));
        append_buffer(buff, &cname, sizeof(cname));
        append_buffer(buff, &cname_padding, sizeof(cname_padding));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

        {
            SdesTraverser sdes_tr = it.get_sdes();
            CHECK(!sdes_tr.parse());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
}

TEST(traverser, sdes_fields) {
    core::Slice<uint8_t> buff = new_buffer();

    {
        const char chunk1_cname[3] = { 'a', 'a', 'a' };
        const char chunk1_email[6] = { 'b', 'b', 'b', 'b', 'b', 'b' };
        const char chunk1_padding[3] = { '\0', 'x', 'x' };

        const char chunk2_cname[4] = { 'c', 'c', 'c', 'c' };
        const char chunk2_email[8] = { 'd', 'd', 'd', 'd', 'd', 'd', 'd', 'd' };
        const char chunk2_padding[4] = { '\0', 'y', 'y', 'y' };

        header::SdesPacket sdes;
        sdes.header().set_counter(2);
        sdes.header().set_len_bytes(
            sizeof(header::SdesPacket)
            // chunk 1
            + sizeof(header::SdesChunkHeader) + sizeof(header::SdesItemHeader)
            + sizeof(chunk1_cname) + sizeof(header::SdesItemHeader) + sizeof(chunk1_email)
            + sizeof(chunk1_padding)
            // chunk 2
            + sizeof(header::SdesChunkHeader) + sizeof(header::SdesItemHeader)
            + sizeof(chunk2_cname) + sizeof(header::SdesItemHeader) + sizeof(chunk2_email)
            + sizeof(chunk2_padding));

        header::SdesChunkHeader chunk1;
        chunk1.set_ssrc(111);

        header::SdesItemHeader chunk1_item;
        chunk1_item.set_type(header::SDES_CNAME);
        chunk1_item.set_text_len(sizeof(chunk1_cname));

        header::SdesItemHeader chunk1_item2;
        chunk1_item2.set_type(header::SDES_EMAIL);
        chunk1_item2.set_text_len(sizeof(chunk1_email));

        header::SdesChunkHeader chunk2;
        chunk2.set_ssrc(222);

        header::SdesItemHeader chunk2_item;
        chunk2_item.set_type(header::SDES_CNAME);
        chunk2_item.set_text_len(sizeof(chunk2_cname));

        header::SdesItemHeader chunk2_item2;
        chunk2_item2.set_type(header::SDES_EMAIL);
        chunk2_item2.set_text_len(sizeof(chunk2_email));

        append_buffer(buff, &sdes, sizeof(sdes));
        append_buffer(buff, &chunk1, sizeof(chunk1));
        append_buffer(buff, &chunk1_item, sizeof(chunk1_item));
        append_buffer(buff, &chunk1_cname, sizeof(chunk1_cname));
        append_buffer(buff, &chunk1_item2, sizeof(chunk1_item2));
        append_buffer(buff, &chunk1_email, sizeof(chunk1_email));
        append_buffer(buff, &chunk1_padding, sizeof(chunk1_padding));
        append_buffer(buff, &chunk2, sizeof(chunk2));
        append_buffer(buff, &chunk2_item, sizeof(chunk2_item));
        append_buffer(buff, &chunk2_cname, sizeof(chunk2_cname));
        append_buffer(buff, &chunk2_item2, sizeof(chunk2_item2));
        append_buffer(buff, &chunk2_email, sizeof(chunk2_email));
        append_buffer(buff, &chunk2_padding, sizeof(chunk2_padding));
    }

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

    {
        SdesTraverser sdes_tr = it.get_sdes();
        CHECK(sdes_tr.parse());

        CHECK_EQUAL(2, sdes_tr.chunks_count());

        SdesTraverser::Iterator sdes_it = sdes_tr.iter();

        CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
        CHECK_EQUAL(111, sdes_it.get_chunk().ssrc);
        CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
        CHECK_EQUAL(header::SDES_CNAME, sdes_it.get_item().type);
        STRCMP_EQUAL("aaa", sdes_it.get_item().text);
        CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
        CHECK_EQUAL(header::SDES_EMAIL, sdes_it.get_item().type);
        STRCMP_EQUAL("bbbbbb", sdes_it.get_item().text);

        CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
        CHECK_EQUAL(222, sdes_it.get_chunk().ssrc);
        CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
        CHECK_EQUAL(header::SDES_CNAME, sdes_it.get_item().type);
        STRCMP_EQUAL("cccc", sdes_it.get_item().text);
        CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
        CHECK_EQUAL(header::SDES_EMAIL, sdes_it.get_item().type);
        STRCMP_EQUAL("dddddddd", sdes_it.get_item().text);

        CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
        CHECK_FALSE(sdes_it.error());
    }

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(traverser, bye_iteration) {
    const char reason_text[5] = { '1', '2', '3', '4', '5' };
    const char reason_padding[2] = { 'x', 'x' };

    header::ByePacket bye;
    bye.header().set_counter(2);
    bye.header().set_len_bytes(
        sizeof(header::ByePacket) + sizeof(header::ByeSourceHeader) * 2
        + sizeof(header::ByeReasonHeader) + sizeof(reason_text) + sizeof(reason_padding));

    header::ByeSourceHeader src1;
    src1.set_ssrc(111);
    header::ByeSourceHeader src2;
    src2.set_ssrc(222);

    header::ByeReasonHeader reason;
    reason.set_text_len(sizeof(reason_text));

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &bye, sizeof(bye));
        append_buffer(buff, &src1, sizeof(src1));
        append_buffer(buff, &src2, sizeof(src2));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(bye_tr.parse());

            CHECK_EQUAL(2, bye_tr.ssrc_count());

            ByeTraverser::Iterator bye_it = bye_tr.iter();

            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(111, bye_it.get_ssrc());
            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(222, bye_it.get_ssrc());
            CHECK_EQUAL(ByeTraverser::Iterator::REASON, bye_it.next());
            STRCMP_EQUAL("12345", bye_it.get_reason());

            CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
            CHECK_FALSE(bye_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // truncated buffer (header)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &bye, sizeof(bye) - 1);

        Traverser traverser(buff);
        CHECK(!traverser.parse());
    }
    { // truncated buffer (body)
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &bye, sizeof(bye));
        append_buffer(buff, &src1, sizeof(src1));
        append_buffer(buff, &src2, sizeof(src2));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding) - 1);

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (header)
        core::Slice<uint8_t> buff = new_buffer();

        header::ByePacket bye_copy = bye;
        bye_copy.header().set_len_words(
            header::size_t_2_rtcp_length(sizeof(header::ByePacket)) - 1);

        append_buffer(buff, &bye_copy, sizeof(bye_copy));
        append_buffer(buff, &src1, sizeof(src1));
        append_buffer(buff, &src2, sizeof(src2));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (ssrc)
        core::Slice<uint8_t> buff = new_buffer();

        header::ByePacket bye_copy = bye;
        bye_copy.header().set_len_words(bye.header().len_words() - 3);

        append_buffer(buff, &bye_copy, sizeof(bye_copy));
        append_buffer(buff, &src1, sizeof(src1));
        append_buffer(buff, &src2, sizeof(src2));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(bye_tr.parse());

            CHECK_EQUAL(2, bye_tr.ssrc_count());

            ByeTraverser::Iterator bye_it = bye_tr.iter();

            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(111, bye_it.get_ssrc());

            CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
            CHECK_TRUE(bye_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // truncated len (reason)
        core::Slice<uint8_t> buff = new_buffer();

        header::ByePacket bye_copy = bye;
        bye_copy.header().set_len_words(bye.header().len_words() - 1);

        append_buffer(buff, &bye_copy, sizeof(bye_copy));
        append_buffer(buff, &src1, sizeof(src1));
        append_buffer(buff, &src2, sizeof(src2));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(bye_tr.parse());

            CHECK_EQUAL(2, bye_tr.ssrc_count());

            ByeTraverser::Iterator bye_it = bye_tr.iter();

            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(111, bye_it.get_ssrc());
            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(222, bye_it.get_ssrc());

            CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
            CHECK_TRUE(bye_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_TRUE(it.error());
    }
    { // no sources
        core::Slice<uint8_t> buff = new_buffer();

        header::ByePacket bye_copy = bye;
        bye_copy.header().set_counter(0);
        bye_copy.header().set_len_bytes(sizeof(header::ByePacket)
                                        + sizeof(header::ByeReasonHeader)
                                        + sizeof(reason_text) + sizeof(reason_padding));

        append_buffer(buff, &bye_copy, sizeof(bye_copy));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(bye_tr.parse());

            CHECK_EQUAL(0, bye_tr.ssrc_count());

            ByeTraverser::Iterator bye_it = bye_tr.iter();

            CHECK_EQUAL(ByeTraverser::Iterator::REASON, bye_it.next());
            STRCMP_EQUAL("12345", bye_it.get_reason());

            CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
            CHECK_FALSE(bye_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // no reason
        core::Slice<uint8_t> buff = new_buffer();

        header::ByePacket bye_copy = bye;
        bye_copy.header().set_len_bytes(sizeof(header::ByePacket)
                                        + sizeof(header::ByeSourceHeader) * 2);

        append_buffer(buff, &bye_copy, sizeof(bye_copy));
        append_buffer(buff, &src1, sizeof(src1));
        append_buffer(buff, &src2, sizeof(src2));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(bye_tr.parse());

            CHECK_EQUAL(2, bye_tr.ssrc_count());

            ByeTraverser::Iterator bye_it = bye_tr.iter();

            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(111, bye_it.get_ssrc());
            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(222, bye_it.get_ssrc());

            CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
            CHECK_FALSE(bye_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // no sources and no reason
        core::Slice<uint8_t> buff = new_buffer();

        header::ByePacket bye_copy = bye;
        bye_copy.header().set_counter(0);
        bye_copy.header().set_len_bytes(sizeof(header::ByePacket));

        append_buffer(buff, &bye_copy, sizeof(bye_copy));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(bye_tr.parse());

            CHECK_EQUAL(0, bye_tr.ssrc_count());

            ByeTraverser::Iterator bye_it = bye_tr.iter();

            CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
            CHECK_FALSE(bye_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // zero-length reason
        core::Slice<uint8_t> buff = new_buffer();

        char padding[3] = {};

        header::ByePacket bye_copy = bye;
        bye_copy.header().set_len_bytes(
            sizeof(header::ByePacket) + sizeof(header::ByeSourceHeader) * 2
            + sizeof(header::ByeReasonHeader) + sizeof(padding));

        header::ByeReasonHeader reason_copy;
        reason_copy.set_text_len(0);

        append_buffer(buff, &bye_copy, sizeof(bye_copy));
        append_buffer(buff, &src1, sizeof(src1));
        append_buffer(buff, &src2, sizeof(src2));
        append_buffer(buff, &reason_copy, sizeof(reason_copy));
        append_buffer(buff, &padding, sizeof(padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(bye_tr.parse());

            CHECK_EQUAL(2, bye_tr.ssrc_count());

            ByeTraverser::Iterator bye_it = bye_tr.iter();

            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(111, bye_it.get_ssrc());
            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(222, bye_it.get_ssrc());
            CHECK_EQUAL(ByeTraverser::Iterator::REASON, bye_it.next());
            STRCMP_EQUAL("", bye_it.get_reason());

            CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
            CHECK_FALSE(bye_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
}

TEST(traverser, bye_padding) {
    char packet_padding[16] = {};
    packet_padding[15] = 16;

    const char reason_text[5] = { '1', '2', '3', '4', '5' };
    const char reason_padding[2] = { 'x', 'x' };

    header::ByePacket bye;
    bye.header().set_padding(true);
    bye.header().set_counter(1);
    bye.header().set_len_bytes(sizeof(header::ByePacket) + sizeof(header::ByeSourceHeader)
                               + sizeof(header::ByeReasonHeader) + sizeof(reason_text)
                               + sizeof(reason_padding) + sizeof(packet_padding));

    header::ByeSourceHeader src;
    src.set_ssrc(111);

    header::ByeReasonHeader reason;
    reason.set_text_len(sizeof(reason_text));

    { // good
        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &bye, sizeof(bye));
        append_buffer(buff, &src, sizeof(src));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));
        append_buffer(buff, &packet_padding, sizeof(packet_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(bye_tr.parse());

            CHECK_EQUAL(1, bye_tr.ssrc_count());

            ByeTraverser::Iterator bye_it = bye_tr.iter();

            CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
            CHECK_EQUAL(111, bye_it.get_ssrc());
            CHECK_EQUAL(ByeTraverser::Iterator::REASON, bye_it.next());
            STRCMP_EQUAL("12345", bye_it.get_reason());

            CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
            CHECK_FALSE(bye_it.error());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // padding length is zero
        char bad_padding[16] = {};
        bad_padding[15] = 0;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &bye, sizeof(bye));
        append_buffer(buff, &src, sizeof(src));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(!bye_tr.parse());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
    { // padding length is too big
        char bad_padding[16] = {};
        bad_padding[15] = 127;

        core::Slice<uint8_t> buff = new_buffer();

        append_buffer(buff, &bye, sizeof(bye));
        append_buffer(buff, &src, sizeof(src));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));
        append_buffer(buff, &bad_padding, sizeof(bad_padding));

        Traverser traverser(buff);
        CHECK(traverser.parse());

        Traverser::Iterator it = traverser.iter();
        CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

        {
            ByeTraverser bye_tr = it.get_bye();
            CHECK(!bye_tr.parse());
        }

        CHECK_EQUAL(Traverser::Iterator::END, it.next());
        CHECK_FALSE(it.error());
    }
}

TEST(traverser, bye_fields) {
    core::Slice<uint8_t> buff = new_buffer();

    {
        const char reason_text[5] = { 'a', 'b', 'c', 'd', 'e' };
        const char reason_padding[2] = { 'x', 'x' };

        header::ByePacket bye;
        bye.header().set_counter(2);
        bye.header().set_len_bytes(sizeof(header::ByePacket)
                                   + sizeof(header::ByeSourceHeader) * 2
                                   + sizeof(header::ByeReasonHeader) + sizeof(reason_text)
                                   + sizeof(reason_padding));

        header::ByeSourceHeader src1;
        src1.set_ssrc(111);
        header::ByeSourceHeader src2;
        src2.set_ssrc(222);

        header::ByeReasonHeader reason;
        reason.set_text_len(sizeof(reason_text));

        append_buffer(buff, &bye, sizeof(bye));
        append_buffer(buff, &src1, sizeof(src1));
        append_buffer(buff, &src2, sizeof(src2));
        append_buffer(buff, &reason, sizeof(reason));
        append_buffer(buff, &reason_text, sizeof(reason_text));
        append_buffer(buff, &reason_padding, sizeof(reason_padding));
    }

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();
    CHECK_EQUAL(Traverser::Iterator::BYE, it.next());

    {
        ByeTraverser bye_tr = it.get_bye();
        CHECK(bye_tr.parse());

        CHECK_EQUAL(2, bye_tr.ssrc_count());

        ByeTraverser::Iterator bye_it = bye_tr.iter();

        CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
        CHECK_EQUAL(111, bye_it.get_ssrc());
        CHECK_EQUAL(ByeTraverser::Iterator::SSRC, bye_it.next());
        CHECK_EQUAL(222, bye_it.get_ssrc());
        CHECK_EQUAL(ByeTraverser::Iterator::REASON, bye_it.next());
        STRCMP_EQUAL("abcde", bye_it.get_reason());

        CHECK_EQUAL(ByeTraverser::Iterator::END, bye_it.next());
        CHECK_FALSE(bye_it.error());
    }

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

TEST(traverser, multiple_packets) {
    core::Slice<uint8_t> buff = new_buffer();

    {
        header::ReceiverReportPacket rr;
        rr.header().set_counter(2);
        rr.header().set_len_bytes(sizeof(header::ReceiverReportPacket)
                                  + sizeof(header::ReceptionReportBlock) * 2);
        rr.set_ssrc(111);

        header::ReceptionReportBlock rr_block1;
        rr_block1.set_ssrc(222);
        header::ReceptionReportBlock rr_block2;
        rr_block2.set_ssrc(333);

        const char cname[4] = { 'a', 'b', 'c', 'd' };
        const char cname_padding[2] = { '\0', 'x' };

        header::SdesPacket sdes;
        sdes.header().set_counter(1);
        sdes.header().set_len_bytes(
            sizeof(header::SdesPacket) + sizeof(header::SdesChunkHeader)
            + sizeof(header::SdesItemHeader) + sizeof(cname) + sizeof(cname_padding));

        header::SdesChunkHeader sdes_chunk;
        sdes_chunk.set_ssrc(444);

        header::SdesItemHeader sdes_item;
        sdes_item.set_type(header::SDES_CNAME);
        sdes_item.set_text_len(sizeof(cname));

        char xr_padding[32] = {};
        xr_padding[31] = 32;

        header::XrPacket xr;
        xr.header().set_padding(true);
        xr.header().set_len_bytes(sizeof(header::XrPacket) + sizeof(header::XrRrtrBlock)
                                  + sizeof(header::XrDlrrBlock)
                                  + sizeof(header::XrDlrrSubblock) * 2
                                  + sizeof(xr_padding));
        xr.set_ssrc(555);

        header::XrRrtrBlock rrtr;
        rrtr.header().set_type_specific(66);
        rrtr.header().set_len_bytes(sizeof(header::XrRrtrBlock));

        header::XrDlrrBlock dlrr;
        dlrr.header().set_type_specific(77);
        dlrr.header().set_len_bytes(sizeof(header::XrDlrrBlock)
                                    + sizeof(header::XrDlrrSubblock) * 2);

        header::XrDlrrSubblock dlrr_subblock1;
        dlrr_subblock1.set_ssrc(888);

        header::XrDlrrSubblock dlrr_subblock2;
        dlrr_subblock2.set_ssrc(999);

        append_buffer(buff, &rr, sizeof(rr));
        append_buffer(buff, &rr_block1, sizeof(rr_block1));
        append_buffer(buff, &rr_block2, sizeof(rr_block2));
        append_buffer(buff, &sdes, sizeof(sdes));
        append_buffer(buff, &sdes_chunk, sizeof(sdes_chunk));
        append_buffer(buff, &sdes_item, sizeof(sdes_item));
        append_buffer(buff, &cname, sizeof(cname));
        append_buffer(buff, &cname_padding, sizeof(cname_padding));
        append_buffer(buff, &xr, sizeof(xr));
        append_buffer(buff, &rrtr, sizeof(rrtr));
        append_buffer(buff, &dlrr, sizeof(dlrr));
        append_buffer(buff, &dlrr_subblock1, sizeof(dlrr_subblock1));
        append_buffer(buff, &dlrr_subblock2, sizeof(dlrr_subblock2));
        append_buffer(buff, &xr_padding, sizeof(xr_padding));
    }

    Traverser traverser(buff);
    CHECK(traverser.parse());

    Traverser::Iterator it = traverser.iter();

    CHECK_EQUAL(Traverser::Iterator::RR, it.next());
    CHECK_EQUAL(111, it.get_rr().ssrc());
    CHECK_EQUAL(2, it.get_rr().num_blocks());
    CHECK_EQUAL(222, it.get_rr().get_block(0).ssrc());
    CHECK_EQUAL(333, it.get_rr().get_block(1).ssrc());

    CHECK_EQUAL(Traverser::Iterator::SDES, it.next());

    {
        SdesTraverser sdes_tr = it.get_sdes();
        CHECK(sdes_tr.parse());

        CHECK_EQUAL(1, sdes_tr.chunks_count());

        SdesTraverser::Iterator sdes_it = sdes_tr.iter();

        CHECK_EQUAL(SdesTraverser::Iterator::CHUNK, sdes_it.next());
        CHECK_EQUAL(444, sdes_it.get_chunk().ssrc);
        CHECK_EQUAL(SdesTraverser::Iterator::ITEM, sdes_it.next());
        STRCMP_EQUAL("abcd", sdes_it.get_item().text);

        CHECK_EQUAL(SdesTraverser::Iterator::END, sdes_it.next());
        CHECK_FALSE(sdes_it.error());
    }

    CHECK_EQUAL(Traverser::Iterator::XR, it.next());

    {
        XrTraverser xr_tr = it.get_xr();
        CHECK(xr_tr.parse());

        CHECK_EQUAL(2, xr_tr.blocks_count());
        CHECK_EQUAL(555, xr_tr.packet().ssrc());

        XrTraverser::Iterator xr_it = xr_tr.iter();
        CHECK_EQUAL(XrTraverser::Iterator::RRTR_BLOCK, xr_it.next());
        CHECK_EQUAL(66, xr_it.get_rrtr().header().type_specific());
        CHECK_EQUAL(XrTraverser::Iterator::DLRR_BLOCK, xr_it.next());
        CHECK_EQUAL(2, xr_it.get_dlrr().num_subblocks());
        CHECK_EQUAL(77, xr_it.get_dlrr().header().type_specific());
        CHECK_EQUAL(888, xr_it.get_dlrr().get_subblock(0).ssrc());
        CHECK_EQUAL(999, xr_it.get_dlrr().get_subblock(1).ssrc());
        CHECK_EQUAL(XrTraverser::Iterator::END, xr_it.next());
        CHECK_FALSE(xr_it.error());
    }

    CHECK_EQUAL(Traverser::Iterator::END, it.next());
    CHECK_FALSE(it.error());
}

} // namespace rtcp
} // namespace roc
