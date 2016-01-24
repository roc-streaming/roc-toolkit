/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_PACKET_STREAM_H_
#define ROC_PIPELINE_TEST_PACKET_STREAM_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/print_buffer.h"
#include "roc_core/log.h"
#include "roc_core/math.h"
#include "roc_datagram/address.h"
#include "roc_packet/units.h"

#include "roc_datagram/idatagram_writer.h"
#include "roc_datagram/idatagram_reader.h"

#include "test_helpers.h"
#include "test_config.h"
#include "test_datagram.h"

namespace roc {
namespace test {

class PacketStream {
public:
    enum { SrcPort = 1000, DstPort = 2000, MaxSamples = 1000 };

    datagram::port_t dst;
    datagram::port_t src;

    long value;

    packet::source_t sid;
    packet::seqnum_t sn;
    packet::timestamp_t ts;

    PacketStream()
        : dst(DstPort)
        , src(SrcPort)
        , value(1)
        , sid(0)
        , sn(0)
        , ts(0) {
    }

    void write(datagram::IDatagramWriter& writer,
               size_t n_packets,
               size_t n_samples_in_packet) {
        for (size_t p = 0; p < n_packets; p++) {
            writer.write(make(n_samples_in_packet));
            sn++;
            ts += n_samples_in_packet;
            value += n_samples_in_packet;
        }
    }

    void read(datagram::IDatagramReader& reader, size_t n_pkt_samples) {
        CHECK(n_pkt_samples < MaxSamples);

        datagram::IDatagramConstPtr dgm = reader.read();
        CHECK(dgm);

        CHECK(dgm->sender() == new_address(SrcPort));
        CHECK(dgm->receiver() == new_address(DstPort));

        packet::IPacketConstPtr packet = parse_packet(*dgm);
        CHECK(packet);

        if (value == 1) {
            sn = packet->rtp()->seqnum();
            ts = packet->rtp()->timestamp();
        }

        LONGS_EQUAL(sn, packet->rtp()->seqnum());
        LONGS_EQUAL(ts, packet->rtp()->timestamp());
        LONGS_EQUAL(ChannelMask, packet->audio()->channels());
        LONGS_EQUAL(n_pkt_samples, packet->audio()->num_samples());

        packet::sample_t samples[MaxSamples * NumChannels] = {};
        LONGS_EQUAL(n_pkt_samples, packet->audio()->read_samples(ChannelMask, 0, samples,
                                                                 n_pkt_samples));

        size_t pos = 0;
        for (size_t n = 0; n < n_pkt_samples; n++) {
            packet::sample_t s =
                packet::sample_t(value % MaxSampleValue) / MaxSampleValue;

            expect_sample(samples, n_pkt_samples, pos++, -s);
            expect_sample(samples, n_pkt_samples, pos++, +s);
            value++;
        }

        sn++;
        ts += n_pkt_samples;
    }

    void read_eof(datagram::IDatagramReader& reader) {
        CHECK(reader.read() == NULL);
    }

    datagram::IDatagramPtr make(size_t n_pkt_samples) const {
        CHECK(n_pkt_samples < MaxSamples);

        packet::IPacketPtr packet = new_packet();

        packet::sample_t samples[MaxSamples * NumChannels] = {};
        size_t pos = 0;
        long v = value;
        for (size_t n = 0; n < n_pkt_samples; n++) {
            packet::sample_t s = packet::sample_t(v % MaxSampleValue) / MaxSampleValue;
            samples[pos++] = -s;
            samples[pos++] = +s;
            v++;
        }

        packet->rtp()->set_source(sid);
        packet->rtp()->set_seqnum(sn);
        packet->rtp()->set_timestamp(ts);
        packet->audio()->configure(ChannelMask, n_pkt_samples, SampleRate);
        packet->audio()->write_samples(ChannelMask, 0, samples, n_pkt_samples);

        return make(packet->raw_data());
    }

    datagram::IDatagramPtr make(core::IByteBufferConstSlice buffer) const {
        datagram::IDatagramPtr dgm = new TestDatagram;

        dgm->set_buffer(buffer);
        dgm->set_sender(new_address(src));
        dgm->set_receiver(new_address(dst));

        return dgm;
    }

private:
    void expect_sample(packet::sample_t* samples,
                       size_t n_samples,
                       size_t pos,
                       packet::sample_t expected) {
        const float Epsilon = 0.0001f;

        packet::sample_t actual = samples[pos];

        if (ROC_ABS(actual - expected) > Epsilon) {
            roc_log(LogError, "unexpected sample at pos %u", (unsigned)pos);
            core::print_buffer(samples, n_samples, n_samples);
        }

        DOUBLES_EQUAL(expected, actual, Epsilon);
    }
};

} // namespace test
} // namespace roc

#endif // ROC_PIPELINE_TEST_PACKET_STREAM_H_
