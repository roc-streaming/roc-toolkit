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
#include "roc_pipeline/sender.h"

#include "test_packet_stream.h"
#include "test_sample_stream.h"
#include "test_sample_queue.h"
#include "test_datagram.h"

namespace roc {
namespace test {

using namespace pipeline;

TEST_GROUP(sender) {
    enum {
        // No FEC and interleaving.
        SenderOptions = 0,

        // Number of samples in every channel per packet.
        PktSamples = 33,

        // Maximum number of sample buffers.
        MaxBuffers = 100
    };

    SampleQueue<MaxBuffers> input;

    datagram::DatagramQueue output;

    rtp::Composer packet_composer;

    TestDatagramComposer datagram_composer;

    SenderConfig config;
    core::ScopedPtr<Sender> sender;

    void setup() {
        config.options = SenderOptions;
        config.channels = ChannelMask;
        config.samples_per_packet = PktSamples;

        sender.reset(
            new Sender(input, output, datagram_composer, packet_composer, config));

        sender->set_sender(new_address(PacketStream::SrcPort));
        sender->set_receiver(new_address(PacketStream::DstPort));
    }

    void teardown() {
        LONGS_EQUAL(0, input.size());
    }
};

TEST(sender, buffer_size_is_packet_size) {
    PacketStream ps;
    ps.read_eof(output);

    SampleStream ss;

    for (size_t n = 0; n < MaxBuffers; n++) {
        ss.write(input, PktSamples);

        CHECK(sender->tick());

        ps.read(output, PktSamples);
        ps.read_eof(output);
    }
}

TEST(sender, buffer_size_larger_than_packet_size) {
    enum {
        WriteBufsz = PktSamples + 3, //
        NumPackets = MaxBuffers * WriteBufsz / PktSamples
    };

    SampleStream ss;

    for (size_t n = 0; n < MaxBuffers; n++) {
        ss.write(input, WriteBufsz);

        CHECK(sender->tick());
    }

    PacketStream ps;

    for (size_t n = 0; n < NumPackets; n++) {
        ps.read(output, PktSamples);
    }

    ps.read_eof(output);
}

TEST(sender, input_eof) {
    PacketStream ps;
    SampleStream ss;

    ss.write(input, PktSamples);
    CHECK(sender->tick() == true);
    ps.read(output, PktSamples);

    input.write(audio::ISampleBufferConstSlice());
    CHECK(sender->tick() == false);
    ps.read_eof(output);

    ss.write(input, PktSamples);
    CHECK(sender->tick() == true);
    ps.read(output, PktSamples);
}

} // namespace test
} // namespace roc
