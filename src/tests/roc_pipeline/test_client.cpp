/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/scoped_ptr.h"
#include "roc_rtp/composer.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_pipeline/client.h"

#include "test_packet_stream.h"
#include "test_sample_stream.h"
#include "test_sample_queue.h"
#include "test_datagram.h"

namespace roc {
namespace test {

using namespace pipeline;

TEST_GROUP(client) {
    enum {
        // No FEC and interleaving.
        ClientOptions = 0,

        // Number of samples in every channel per packet.
        PktSamples = 33,

        // Maximum number of sample buffers.
        MaxBuffers = 100
    };

    SampleQueue<MaxBuffers> input;

    datagram::DatagramQueue output;

    rtp::Composer packet_composer;

    TestDatagramComposer datagram_composer;

    ClientConfig config;
    core::ScopedPtr<Client> client;

    void setup() {
        config.options = ClientOptions;
        config.channels = ChannelMask;
        config.samples_per_packet = PktSamples;

        client.reset(
            new Client(input, output, datagram_composer, packet_composer, config));

        client->set_sender(new_address(PacketStream::SrcPort));
        client->set_receiver(new_address(PacketStream::DstPort));
    }

    void teardown() {
        LONGS_EQUAL(0, input.size());
    }
};

TEST(client, buffer_size_is_packet_size) {
    PacketStream ps;
    ps.read_eof(output);

    SampleStream ss;

    for (size_t n = 0; n < MaxBuffers; n++) {
        ss.write(input, PktSamples);

        CHECK(client->tick());

        ps.read(output, PktSamples);
        ps.read_eof(output);
    }
}

TEST(client, buffer_size_larger_than_packet_size) {
    enum {
        WriteBufsz = PktSamples + 3, //
        NumPackets = MaxBuffers * WriteBufsz / PktSamples
    };

    SampleStream ss;

    for (size_t n = 0; n < MaxBuffers; n++) {
        ss.write(input, WriteBufsz);

        CHECK(client->tick());
    }

    PacketStream ps;

    for (size_t n = 0; n < NumPackets; n++) {
        ps.read(output, PktSamples);
    }

    ps.read_eof(output);
}

TEST(client, input_eof) {
    PacketStream ps;
    SampleStream ss;

    ss.write(input, PktSamples);
    CHECK(client->tick() == true);
    ps.read(output, PktSamples);

    input.write(audio::ISampleBufferConstSlice());
    CHECK(client->tick() == false);
    ps.read_eof(output);

    ss.write(input, PktSamples);
    CHECK(client->tick() == true);
    ps.read(output, PktSamples);
}

} // namespace test
} // namespace roc
