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

rtp::PayloadType PayloadType = rtp::PayloadType_L16_Stereo;

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
    // enable FEC on sender or receiver
    FlagFEC = (1 << 0),

    // enable interleaving on sender
    FlagInterleaving = (1 << 1),

    // enable packet loss on sender
    FlagLoss = (1 << 2)
};

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, 1);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, 1);
packet::PacketPool packet_pool(allocator, 1);
rtp::FormatMap format_map;
} // namespace

TEST_GROUP(sender_receiver) {
    void send_receive(int flags) {
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
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
    }

    void transfer_packets(int flags, packet::IReader& reader, packet::IWriter& writer) {
        size_t counter = 0;

        while (packet::PacketPtr pp = reader.read()) {
            if ((flags & FlagLoss) && counter++ % (SourcePackets + RepairPackets) == 1) {
                continue;
            }

            writer.write(convert_packet(pp));
        }
    }

    packet::PacketPtr convert_packet(const packet::PacketPtr& pa) {
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
        config.default_session.payload_type = PayloadType;

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

    void read_queued_packets(packet::ConcurrentQueue& queue,
            packet::ConcurrentQueue& source_queue,
            packet::ConcurrentQueue& repair_queue) {

        int idx = 0;
        for (;;) {
            if (source_queue.size() == 0 && repair_queue.size() == 0) {
                break;
            }

            if (idx++ % 2 == 0) {
                if (packet::PacketPtr pp = source_queue.read()) {
                    queue.write(pp);
                    continue;
                }

                while (packet::PacketPtr pp = repair_queue.read()) {
                    queue.write(pp);
                }
            } else {
                if (packet::PacketPtr pp = repair_queue.read()) {
                    queue.write(pp);
                    continue;
                }

                while (packet::PacketPtr pp = source_queue.read()) {
                    queue.write(pp);
                }
            }
        }
    }

    void transfer_and_read_packets(int flags, size_t num_sessions, Receiver& receiver,
                                   packet::IReader& reader) {

        transfer_packets(flags, reader, receiver);
        FrameReader frame_reader(receiver, sample_buffer_pool);

        for (size_t nf = 0; nf < ManyFrames; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, num_sessions);
        }

        CHECK(receiver.num_sessions() == num_sessions);
    }
};

TEST(sender_receiver, bare) {
    send_receive(0);
}

TEST(sender_receiver, interleaving) {
    send_receive(FlagInterleaving);
}

#ifdef ROC_TARGET_OPENFEC
TEST(sender_receiver, fec) {
    send_receive(FlagFEC);
}

TEST(sender_receiver, fec_interleaving) {
    send_receive(FlagFEC | FlagInterleaving);
}

TEST(sender_receiver, fec_loss) {
    send_receive(FlagFEC | FlagLoss);
}

TEST(sender_receiver, one_session_drop_leading_repair_packets) {
    packet::ConcurrentQueue queue(0, false);

    PortConfig source_port = source_port_config(FlagFEC);
    PortConfig repair_port = repair_port_config(FlagFEC);

    Sender sender(sender_config(FlagFEC, source_port, repair_port),
                  queue,
                  queue,
                  format_map,
                  packet_pool,
                  byte_buffer_pool,
                  allocator);

    CHECK(sender.valid());

    Receiver receiver(receiver_config(FlagFEC),
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

    packet::ConcurrentQueue source_queue(0, false);
    packet::ConcurrentQueue repair_queue(0, false);

    while (packet::PacketPtr pp = queue.read()) {
        if (pp->flags() & packet::Packet::FlagRepair) {
            repair_queue.write(pp);
        } else {
            source_queue.write(pp);
        }
    }

    queue.write(repair_queue.read());
    transfer_and_read_packets(FlagFEC, 0, receiver, queue);

    read_queued_packets(queue, source_queue, repair_queue);
    transfer_and_read_packets(FlagFEC, 1, receiver, queue);
}
#endif //! ROC_TARGET_OPENFEC

} // namespace pipeline
} // namespace roc
