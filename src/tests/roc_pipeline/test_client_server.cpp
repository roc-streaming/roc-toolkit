/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_config/config.h"
#include "roc_core/scoped_ptr.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/parser.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_pipeline/client.h"
#include "roc_pipeline/server.h"

#include "test_sample_stream.h"
#include "test_sample_queue.h"
#include "test_datagram.h"

namespace roc {
namespace test {

namespace {

const fec::CodecType Fec = fec::ReedSolomon2m;

} // namespace

using namespace pipeline;

TEST_GROUP(client_server) {
    enum {
        // Sending port.
        ClientPort = 501,

        // Receiving port.
        ServerPort = 502,

        // Number of samples in every channel per packet.
        PktSamples = ROC_CONFIG_DEFAULT_PACKET_SAMPLES,

        // Number of samples in input/output buffers.
        BufSamples = SampleStream::ReadBufsz,

        // Number of packets to read per tick.
        PacketsPerTick = 20,

        // Maximum number of sample buffers.
        MaxBuffers = PktSamples * 100 / BufSamples,

        // Percentage of packets to be lost.
        RandomLoss = 1
    };

    SampleQueue<MaxBuffers> input;
    SampleQueue<MaxBuffers> output;

    datagram::DatagramQueue network;
    TestDatagramComposer datagram_composer;

    rtp::Composer packet_composer;
    rtp::Parser packet_parser;

    core::ScopedPtr<Client> client;
    core::ScopedPtr<Server> server;

    void setup() {
    }

    void teardown() {
        LONGS_EQUAL(0, input.size());
        LONGS_EQUAL(0, output.size());
        LONGS_EQUAL(0, network.size());
    }

    void init_client(int options, fec::CodecType codec = fec::NoCodec,
                                size_t random_loss = 0) {
        ClientConfig config;

        config.options = options;
        config.channels = ChannelMask;
        config.samples_per_packet = PktSamples;
        config.random_loss_rate = random_loss;
        config.fec.codec = codec;
        config.fec.n_source_packets = 20;
        config.fec.n_repair_packets = 10;

        client.reset(
            new Client(input, network, datagram_composer, packet_composer, config));

        client->set_sender(new_address(ClientPort));
        client->set_receiver(new_address(ServerPort));
    }

    void init_server(int options, fec::CodecType codec = fec::NoCodec) {
        ServerConfig config;

        config.options = options;
        config.channels = ChannelMask;
        config.session_timeout = MaxBuffers * BufSamples;
        config.session_latency = BufSamples;
        config.output_latency = 0;
        config.samples_per_tick = BufSamples;
        config.fec.codec = codec;
        config.fec.n_source_packets = 20;
        config.fec.n_repair_packets = 10;

        server.reset(new Server(network, output, config));

        server->add_port(new_address(ServerPort), packet_parser);
    }

    void flow_client_server() {
        SampleStream si;

        for (size_t n = 0; n < MaxBuffers; n++) {
            si.write(input, BufSamples);
        }

        LONGS_EQUAL(MaxBuffers, input.size());

        while (input.size() != 0) {
            CHECK(client->tick());
        }

        client->flush();

        CHECK(network.size() >= MaxBuffers * BufSamples / PktSamples);

        SampleStream so;

        for (size_t n = 0; n < MaxBuffers; n++) {
            CHECK(server->tick());

            LONGS_EQUAL(1, output.size());

            so.read(output, BufSamples);

            LONGS_EQUAL(0, output.size());
        }

        LONGS_EQUAL(0, network.size());
    }
};

TEST(client_server, bare) {
    init_client(0);
    init_server(0);
    flow_client_server();
}

TEST(client_server, interleaving) {
    init_client(EnableInterleaving);
    init_server(0);
    flow_client_server();
}

#ifdef ROC_TARGET_OPENFEC
TEST(client_server, fec_only_client) {
    init_client(0, Fec);
    init_server(0);
    flow_client_server();
}

TEST(client_server, fec_only_server) {
    init_client(0);
    init_server(0, Fec);
    flow_client_server();
}

TEST(client_server, fec_both) {
    init_client(0, Fec);
    init_server(0, Fec);
    flow_client_server();
}

TEST(client_server, fec_interleaving) {
    init_client(EnableInterleaving, Fec);
    init_server(0, Fec);
    flow_client_server();
}

TEST(client_server, fec_random_loss) {
    init_client(0, Fec, RandomLoss);
    init_server(0, Fec);
    flow_client_server();
}

TEST(client_server, fec_interleaving_random_loss) {
    init_client(EnableInterleaving, Fec, RandomLoss);
    init_server(0, Fec);
    flow_client_server();
}
#endif // ROC_TARGET_OPENFEC

} // namespace test
} // namespace roc
