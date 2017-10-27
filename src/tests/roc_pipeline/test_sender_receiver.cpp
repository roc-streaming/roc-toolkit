/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"
#include "roc_pipeline/receiver.h"
#include "roc_pipeline/sender.h"
#include "roc_rtp/format_map.h"

#include "test_frame_reader.h"
#include "test_frame_writer.h"

namespace roc {
namespace pipeline {

namespace {

enum {
    MaxBufSize = 4096,

    SampleRate = 44100,
    ChMask = 0x3,
    NumCh = 2,

    SamplesPerFrame = 10,
    SamplesPerPacket = 40,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    SourcePackets = 5,
    RepairPackets = 10,

    Latency = SamplesPerPacket * (SourcePackets + RepairPackets),
    Timeout = Latency * 20,

    ManyFrames = Latency / SamplesPerFrame * 5
};

enum {
    // default flags
    FlagNone = 0,

    // enable FEC on sender or receiver
    FlagFEC = (1 << 0),

    // enable interleaving on sender
    FlagInterleaving = (1 << 1),

    // enable packet loss on sender
    FlagLoss = (1 << 2),

    // drop all source packets
    FlagDropSource = (1 << 3),

    // drop all repair packets
    FlagDropRepair = (1 << 4)
};

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, 1);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, 1);
packet::PacketPool packet_pool(allocator, 1);
rtp::FormatMap format_map;

} // namespace

TEST_GROUP(sender_receiver) {
    void send_receive(int flags, size_t num_sessions) {
        packet::ConcurrentQueue queue(0, false);

        PortConfig source_port = source_port_config(flags);
        PortConfig repair_port = repair_port_config(flags);

        Sender sender(sender_config(flags, source_port, repair_port),
                      queue,
                      queue,
                      format_map,
                      packet_pool,
                      byte_buffer_pool,
                      allocator);

        CHECK(sender.valid());

        Receiver receiver(receiver_config(flags),
                          format_map,
                          packet_pool,
                          byte_buffer_pool,
                          sample_buffer_pool,
                          allocator);

        CHECK(receiver.valid());

        CHECK(receiver.add_port(source_port));
        CHECK(receiver.add_port(repair_port));

        FrameWriter frame_writer(sender, sample_buffer_pool);

        for (size_t nf = 0; nf < ManyFrames; nf++) {
            frame_writer.write_samples(SamplesPerFrame * NumCh);
        }

        transfer_packets(flags, queue, receiver);

        FrameReader frame_reader(receiver, sample_buffer_pool);

        for (size_t nf = 0; nf < ManyFrames; nf++) {
            if (num_sessions == 0) {
                frame_reader.skip_zeros(SamplesPerFrame * NumCh);
            } else {
                frame_reader.read_samples(SamplesPerFrame * NumCh, num_sessions);
            }
            UNSIGNED_LONGS_EQUAL(num_sessions, receiver.num_sessions());
        }
    }

    void transfer_packets(int flags, packet::IReader& reader, packet::IWriter& writer) {
        size_t counter = 0;

        while (packet::PacketPtr pp = reader.read()) {
            if ((flags & FlagLoss) && counter++ % (SourcePackets + RepairPackets) == 1) {
                continue;
            }

            if (pp->flags() & packet::Packet::FlagRepair) {
                if (flags & FlagDropRepair) {
                    continue;
                }
            } else {
                if (flags & FlagDropSource) {
                    continue;
                }
            }

            writer.write(copy_packet(pp));
        }
    }

    packet::PacketPtr copy_packet(const packet::PacketPtr& pa) {
        packet::PacketPtr pb = new (packet_pool) packet::Packet (packet_pool);
        CHECK(pb);

        CHECK(pa->flags() & packet::Packet::FlagUDP);
        pb->add_flags(packet::Packet::FlagUDP);
        *pb->udp() = *pa->udp();

        pb->set_data(pa->data());

        return pb;
    }

    PortConfig source_port_config(int flags) {
        PortConfig port;
        port.address = new_address(1);
        port.protocol = (flags & FlagFEC) ? Proto_RTP_RSm8_Source : Proto_RTP;
        return port;
    }

    PortConfig repair_port_config(int flags) {
        PortConfig port;
        port.address = new_address(2);
        port.protocol = (flags & FlagFEC) ? Proto_RSm8_Repair : Proto_RTP;
        return port;
    }

    SenderConfig sender_config(int flags,
                               const PortConfig& source_port,
                               const PortConfig& repair_port) {
        SenderConfig config;

        config.source_port = source_port;
        config.repair_port = repair_port;

        config.channels = ChMask;
        config.samples_per_packet = SamplesPerPacket;

        config.fec = fec_config(flags);

        config.interleaving = (flags & FlagInterleaving);
        config.timing = false;

        return config;
    }

    ReceiverConfig receiver_config(int flags) {
        ReceiverConfig config;

        config.sample_rate = SampleRate;
        config.channels = ChMask;

        config.default_session.channels = ChMask;
        config.default_session.samples_per_packet = SamplesPerPacket;
        config.default_session.latency = Latency;
        config.default_session.timeout = Timeout;

        config.default_session.fec = fec_config(flags);

        return config;
    }

    fec::Config fec_config(int flags) {
        fec::Config config;

        if (flags & FlagFEC) {
            config.codec = fec::ReedSolomon8m;
            config.n_source_packets = SourcePackets;
            config.n_repair_packets = RepairPackets;
        } else {
            config.codec = fec::NoCodec;
        }

        return config;
    }
};

TEST(sender_receiver, bare) {
    send_receive(FlagNone, 1);
}

TEST(sender_receiver, interleaving) {
    send_receive(FlagInterleaving, 1);
}

#ifdef ROC_TARGET_OPENFEC
TEST(sender_receiver, fec) {
    send_receive(FlagFEC, 1);
}

TEST(sender_receiver, fec_interleaving) {
    send_receive(FlagFEC | FlagInterleaving, 1);
}

TEST(sender_receiver, fec_loss) {
    send_receive(FlagFEC | FlagLoss, 1);
}

TEST(sender_receiver, fec_drop_source) {
    send_receive(FlagFEC | FlagDropSource, 0);
}

TEST(sender_receiver, fec_drop_repair) {
    send_receive(FlagFEC | FlagDropRepair, 1);
}
#endif //! ROC_TARGET_OPENFEC

} // namespace pipeline
} // namespace roc
