/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_CONTROL_WRITER_H_
#define ROC_PIPELINE_TEST_HELPERS_CONTROL_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/ntp.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtcp/builder.h"

namespace roc {
namespace pipeline {
namespace test {

// Generates control packets and pass them to destination writer
class ControlWriter : public core::NonCopyable<> {
public:
    ControlWriter(packet::IWriter& writer,
                  packet::PacketFactory& packet_factory,
                  core::BufferFactory<uint8_t>& buffer_factory,
                  const address::SocketAddr& src_addr,
                  const address::SocketAddr& dst_addr)
        : writer_(writer)
        , packet_factory_(packet_factory)
        , buffer_factory_(buffer_factory)
        , src_addr_(src_addr)
        , dst_addr_(dst_addr)
        , source_(0) {
    }

    void write_sender_report(packet::ntp_timestamp_t ntp_ts,
                             packet::stream_timestamp_t rtp_ts) {
        core::Slice<uint8_t> buff = buffer_factory_.new_buffer();
        CHECK(buff);

        buff.reslice(0, 0);

        rtcp::Config cfg;
        rtcp::Builder bld(cfg, buff);

        rtcp::header::SenderReportPacket sr;
        sr.set_ssrc(source_);
        sr.set_ntp_timestamp(ntp_ts);
        sr.set_rtp_timestamp(rtp_ts);

        rtcp::SdesChunk chunk;
        chunk.ssrc = source_;
        rtcp::SdesItem item;
        item.type = rtcp::header::SDES_CNAME;
        item.text = "test_cname";

        bld.begin_sr(sr);
        bld.end_sr();

        bld.begin_sdes();
        bld.begin_sdes_chunk(chunk);
        bld.add_sdes_item(item);
        bld.end_sdes_chunk();
        bld.end_sdes();

        UNSIGNED_LONGS_EQUAL(status::StatusOK, writer_.write(new_packet_(buff)));
    }

    void set_source(packet::stream_source_t source) {
        source_ = source;
    }

private:
    packet::PacketPtr new_packet_(core::Slice<uint8_t> buffer) {
        packet::PacketPtr pp = packet_factory_.new_packet();
        CHECK(pp);

        pp->add_flags(packet::Packet::FlagUDP);

        pp->udp()->src_addr = src_addr_;
        pp->udp()->dst_addr = dst_addr_;

        pp->set_data(buffer);

        return pp;
    }

    packet::IWriter& writer_;

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& buffer_factory_;

    address::SocketAddr src_addr_;
    address::SocketAddr dst_addr_;

    packet::stream_source_t source_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_CONTROL_WRITER_H_
