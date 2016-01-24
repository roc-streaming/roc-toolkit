/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_rtp/rtp_packet.h"
#include "roc_rtp/parser.h"
#include "roc_datagram/default_buffer_composer.h"

#include "test_blobs/rtp_l16_2ch_320s.h"
#include "test_blobs/rtp_l16_1ch_10s_12ext.h"
#include "test_blobs/rtp_l16_1ch_10s_4pad_2csrc_12ext_marker.h"

namespace roc {
namespace test {

using namespace packet;
using namespace rtp;

namespace {

enum { MaxSize = 2000, MaxSamples = 500 };

const double Epsilon = 0.00001;

} // namespace

TEST_GROUP(rtp_packet) {
    IPacketConstPtr parse(const core::IByteBufferConstSlice& buff) {
        rtp::Parser parser;
        IPacketConstPtr packet = parser.parse(buff);
        CHECK(packet);
        CHECK(packet->rtp());
        CHECK(packet->audio());
        return packet;
    }

    core::IByteBufferPtr make_buffer(const RTP_PacketTest& test) {
        core::IByteBufferPtr buffer =
            core::ByteBufferTraits::default_composer<MaxSize>().compose();

        CHECK(buffer);

        buffer->set_size(test.packet_size);
        memcpy(buffer->data(), test.raw_data, buffer->size());

        return buffer;
    }

    void test_rtp_packet(const RTP_PacketTest& test) {
        core::IByteBufferConstSlice buffer = *make_buffer(test);

        RTP_Packet packet;
        CHECK(packet.parse(buffer));

        CHECK(packet.raw_data());
        LONGS_EQUAL(test.packet_size, packet.raw_data().size());
        LONGS_EQUAL(0, memcmp(buffer.data(), packet.raw_data().data(), test.packet_size));

        LONGS_EQUAL(test.header_size, packet.header().header_size());
        LONGS_EQUAL(test.payload_size, packet.payload().size());

        CHECK_EQUAL(test.packet_size, test.header_size + test.extension_size
                    + test.payload_size + test.padding_size);

        if (test.extension) {
            CHECK(packet.ext_header());
            LONGS_EQUAL(test.ext_type, packet.ext_header()->type());
            LONGS_EQUAL(test.ext_data_size, packet.ext_header()->data_size());
        }

        LONGS_EQUAL(test.version, packet.header().version());
        LONGS_EQUAL(test.padding, packet.header().has_padding());
        LONGS_EQUAL(test.extension, packet.header().has_extension());
        LONGS_EQUAL(test.num_csrc, packet.header().num_csrc());
        LONGS_EQUAL(test.pt, packet.header().payload_type());
        LONGS_EQUAL(test.marker, packet.header().marker());

        LONGS_EQUAL(test.seqnum, packet.header().seqnum());
        LONGS_EQUAL(test.ts, packet.header().timestamp());
        LONGS_EQUAL(test.ssrc, packet.header().ssrc());

        for (size_t n = 0; n < test.num_csrc; n++) {
            LONGS_EQUAL(test.csrc[n], packet.header().csrc(n));
        }
    }

    void test_audio_packet(const RTP_PacketTest& test) {
        core::IByteBufferConstSlice buffer = *make_buffer(test);

        IPacketConstPtr packet = parse(buffer);

        CHECK(packet->raw_data());
        LONGS_EQUAL(test.packet_size, packet->raw_data().size());
        LONGS_EQUAL(
            0, memcmp(buffer.data(), packet->raw_data().data(), test.packet_size));

        LONGS_EQUAL(test.ssrc, packet->rtp()->source());
        LONGS_EQUAL(test.seqnum, packet->rtp()->seqnum());
        LONGS_EQUAL(test.marker, packet->rtp()->marker());
        LONGS_EQUAL(test.ts, packet->rtp()->timestamp());
        LONGS_EQUAL(test.samplerate, packet->rtp()->rate());

        LONGS_EQUAL((1 << test.num_channels) - 1, packet->audio()->channels());
        LONGS_EQUAL(test.num_samples, packet->audio()->num_samples());

        for (size_t ch = 0; ch < test.num_channels; ch++) {
            sample_t samples[MaxSamples] = {};
            LONGS_EQUAL(test.num_samples,
                        packet->audio()->read_samples(
                            (1 << ch), 0, samples, test.num_samples));

            for (size_t ns = 0; ns < test.num_samples; ns++) {
                double s = (double)test.samples[ch][ns] / (1 << (test.samplebits - 1));
                DOUBLES_EQUAL(s, samples[ns], Epsilon);
            }
        }
    }

    void test_packet(const RTP_PacketTest& test) {
        test_rtp_packet(test);
        test_audio_packet(test);
    }
};

TEST(rtp_packet, l16_2ch_320s) {
    test_packet(rtp_l16_2ch_320s);
}

TEST(rtp_packet, l16_1ch_10s_12ext) {
    test_packet(rtp_l16_1ch_10s_12ext);
}

TEST(rtp_packet, l16_1ch_10s_4pad_2csrc_12ext_marker) {
    test_packet(rtp_l16_1ch_10s_4pad_2csrc_12ext_marker);
}

} // namespace test
} // namespace roc
