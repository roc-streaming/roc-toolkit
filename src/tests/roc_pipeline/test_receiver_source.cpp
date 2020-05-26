/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/frame_reader.h"
#include "test_helpers/packet_writer.h"
#include "test_helpers/scheduler.h"

#include "roc_audio/pcm_funcs.h"
#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/time.h"
#include "roc_fec/codec_map.h"
#include "roc_packet/packet_pool.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_rtp/composer.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

namespace {

const rtp::PayloadType PayloadType = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 500,

    SampleRate = 44100,
    ChMask = 0x3,
    NumCh = 2,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    Latency = SamplesPerPacket * 8,
    Timeout = Latency * 13,

    ManyPackets = Latency / SamplesPerPacket * 10,

    MaxSnJump = ManyPackets * 5,
    MaxTsJump = ManyPackets * 7 * SamplesPerPacket
};

const core::nanoseconds_t MaxBufDuration =
    MaxBufSize * core::Second / (SampleRate * packet::num_channels(ChMask));

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, true);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

rtp::FormatMap format_map;
rtp::Composer rtp_composer(NULL);

ReceiverSource::EndpointSetHandle add_endpoint_set(ReceiverSource& receiver) {
    ReceiverSource::Tasks::AddEndpointSet task;
    CHECK(receiver.schedule_and_wait(task));

    CHECK(task.success());
    CHECK(task.get_handle());

    return task.get_handle();
}

packet::IWriter* add_endpoint(ReceiverSource& receiver,
                              ReceiverSource::EndpointSetHandle endpoint_set,
                              address::Interface iface,
                              address::Protocol proto) {
    ReceiverSource::Tasks::CreateEndpoint task(endpoint_set, iface, proto);
    CHECK(receiver.schedule_and_wait(task));

    CHECK(task.success());
    CHECK(task.get_writer());

    return task.get_writer();
}

class TaskIssuer : public TaskPipeline::ICompletionHandler {
public:
    TaskIssuer(TaskPipeline& pipeline)
        : pipeline_(pipeline)
        , endpoint_set_(NULL)
        , task_add_endpoint_set_(NULL)
        , task_create_endpoint_(NULL)
        , task_delete_endpoint_(NULL)
        , done_(false) {
    }

    ~TaskIssuer() {
        delete task_add_endpoint_set_;
        delete task_create_endpoint_;
        delete task_delete_endpoint_;
    }

    void start() {
        task_add_endpoint_set_ = new ReceiverSource::Tasks::AddEndpointSet();
        pipeline_.schedule(*task_add_endpoint_set_, *this);
    }

    void wait_done() const {
        while (!done_) {
            core::sleep_for(core::Microsecond * 10);
        }
    }

    virtual void pipeline_task_finished(TaskPipeline::Task& task) {
        roc_panic_if_not(task.success());

        if (&task == task_add_endpoint_set_) {
            endpoint_set_ = task_add_endpoint_set_->get_handle();
            roc_panic_if_not(endpoint_set_);
            task_create_endpoint_ = new ReceiverSource::Tasks::CreateEndpoint(
                endpoint_set_, address::Iface_AudioSource, address::Proto_RTP);
            pipeline_.schedule(*task_create_endpoint_, *this);
            return;
        }

        if (&task == task_create_endpoint_) {
            task_delete_endpoint_ = new ReceiverSource::Tasks::DeleteEndpoint(
                endpoint_set_, address::Iface_AudioSource);
            pipeline_.schedule(*task_delete_endpoint_, *this);
            return;
        }

        if (&task == task_delete_endpoint_) {
            done_ = true;
            return;
        }

        roc_panic("unexpected task");
    }

private:
    TaskPipeline& pipeline_;

    ReceiverSource::EndpointSetHandle endpoint_set_;

    ReceiverSource::Tasks::AddEndpointSet* task_add_endpoint_set_;
    ReceiverSource::Tasks::CreateEndpoint* task_create_endpoint_;
    ReceiverSource::Tasks::DeleteEndpoint* task_delete_endpoint_;

    core::Atomic<int> done_;
};

} // namespace

TEST_GROUP(receiver_source) {
    test::Scheduler scheduler;

    ReceiverConfig config;

    address::SocketAddr src1;
    address::SocketAddr src2;

    address::SocketAddr dst1;
    address::SocketAddr dst2;

    address::Protocol proto1;
    address::Protocol proto2;

    void setup() {
        config.common.output_sample_rate = SampleRate;
        config.common.output_channels = ChMask;
        config.common.internal_frame_length = MaxBufDuration;

        config.common.resampling = false;
        config.common.timing = false;
        config.common.poisoning = true;
        config.common.profiling = true;

        config.default_session.channels = ChMask;

        config.default_session.target_latency = Latency * core::Second / SampleRate;

        config.default_session.latency_monitor.min_latency =
            -Timeout * 10 * core::Second / SampleRate;
        config.default_session.latency_monitor.max_latency =
            +Timeout * 10 * core::Second / SampleRate;

        config.default_session.watchdog.no_playback_timeout =
            Timeout * core::Second / SampleRate;

        config.default_session.rtp_validator.max_sn_jump = MaxSnJump;
        config.default_session.rtp_validator.max_ts_jump =
            MaxTsJump * core::Second / SampleRate;

        src1 = test::new_address(1);
        src2 = test::new_address(2);

        dst1 = test::new_address(3);
        dst2 = test::new_address(4);

        proto1 = address::Proto_RTP;
        proto2 = address::Proto_RTP;
    }
};

TEST(receiver_source, endpoints_sync) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = NULL;

    {
        ReceiverSource::Tasks::AddEndpointSet task;
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_handle());

        endpoint_set = task.get_handle();
    }

    {
        ReceiverSource::Tasks::CreateEndpoint task(
            endpoint_set, address::Iface_AudioSource, address::Proto_RTP);
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_writer());
    }

    {
        ReceiverSource::Tasks::DeleteEndpoint task(endpoint_set,
                                                   address::Iface_AudioSource);
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
    }
}

TEST(receiver_source, endpoints_async) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    TaskIssuer ti(receiver);

    ti.start();
    ti.wait_done();

    scheduler.wait_done();
}

TEST(receiver_source, no_sessions) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyPackets * FramesPerPacket; nf++) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);

        UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
    }
}

TEST(receiver_source, one_session) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, one_session_long_run) {
    enum { NumIterations = 10 };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (size_t np = 0; np < ManyPackets; np++) {
            for (size_t nf = 0; nf < FramesPerPacket; nf++) {
                frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

                UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
            }

            packet_writer.write_packets(1, SamplesPerPacket, ChMask);
        }
    }
}

TEST(receiver_source, initial_latency) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);

        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.skip_zeros(SamplesPerFrame * NumCh);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    packet_writer.write_packets(1, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }
}

TEST(receiver_source, initial_latency_timeout) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(1, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < Timeout / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.skip_zeros(SamplesPerFrame * NumCh);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    frame_reader.skip_zeros(SamplesPerFrame * NumCh);

    UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
}

TEST(receiver_source, timeout) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }

        UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
    }

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);
    }
}

TEST(receiver_source, initial_trim) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency * 3 / SamplesPerPacket, SamplesPerPacket, ChMask);

    frame_reader.set_offset(Latency * 2 * NumCh);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, two_sessions_synchronous) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer1(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src1, dst1);

    test::PacketWriter packet_writer2(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src2, dst1);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 2);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, two_sessions_overlapping) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer1(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src1, dst1);

    packet_writer1.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
    }

    test::PacketWriter packet_writer2(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src2, dst1);

    packet_writer2.set_offset(packet_writer1.offset() - Latency * NumCh);
    packet_writer2.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 2);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, two_sessions_two_endpoints) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set1 = add_endpoint_set(receiver);
    CHECK(endpoint_set1);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set1, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    ReceiverSource::EndpointSetHandle endpoint_set2 = add_endpoint_set(receiver);
    CHECK(endpoint_set2);

    packet::IWriter* endpoint2_writer =
        add_endpoint(receiver, endpoint_set2, address::Iface_AudioSource, proto2);
    CHECK(endpoint2_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer1(allocator, *endpoint1_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src1, dst1);

    test::PacketWriter packet_writer2(allocator, *endpoint2_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src2, dst2);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 2);

            UNSIGNED_LONGS_EQUAL(2, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, two_sessions_same_address_same_stream) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer1(allocator, *endpoint_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src1, dst1);

    test::PacketWriter packet_writer2(allocator, *endpoint_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src1, dst2);

    packet_writer1.set_source(11);
    packet_writer2.set_source(11);

    packet_writer2.set_offset(77);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, two_sessions_same_address_different_streams) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer1(allocator, *endpoint_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src1, dst1);

    test::PacketWriter packet_writer2(allocator, *endpoint_writer, rtp_composer,
                                      format_map, packet_pool, byte_buffer_pool,
                                      PayloadType, src1, dst2);

    packet_writer1.set_source(11);
    packet_writer2.set_source(22);

    packet_writer2.set_offset(77);
    packet_writer2.set_seqnum(5);
    packet_writer2.set_timestamp(5 * SamplesPerPacket);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer1.write_packets(1, SamplesPerPacket, ChMask);
        packet_writer2.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, seqnum_overflow) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.set_seqnum(packet::seqnum_t(-1) - ManyPackets / 2);
    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, seqnum_small_jump) {
    enum { SmallJump = 5 };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    packet_writer.set_seqnum(packet_writer.seqnum() + SmallJump);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, seqnum_large_jump) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    packet_writer.set_seqnum(packet_writer.seqnum() + MaxSnJump);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);
    }
}

TEST(receiver_source, seqnum_reorder) {
    enum { ReorderWindow = Latency / SamplesPerPacket };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    size_t pos = 0;

    for (size_t ni = 0; ni < ManyPackets / ReorderWindow; ni++) {
        if (pos >= Latency / SamplesPerPacket) {
            for (size_t nf = 0; nf < ReorderWindow * FramesPerPacket; nf++) {
                frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
            }
        }

        for (ssize_t np = ReorderWindow - 1; np >= 0; np--) {
            packet_writer.shift_to(pos + size_t(np), SamplesPerPacket, ChMask);
            packet_writer.write_packets(1, SamplesPerPacket, ChMask);
        }

        pos += ReorderWindow;
    }
}

TEST(receiver_source, seqnum_late) {
    enum { DelayedPackets = 5 };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);
    packet_writer.shift_to(Latency / SamplesPerPacket + DelayedPackets, SamplesPerPacket,
                           ChMask);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < DelayedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
        }
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    packet_writer.shift_to(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);
    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
    }

    frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
}

TEST(receiver_source, timestamp_overflow) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.set_timestamp(packet::timestamp_t(-1)
                                - ManyPackets * SamplesPerPacket / 2);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, timestamp_small_jump) {
    enum { ShiftedPackets = 5 };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    packet_writer.set_timestamp(Latency + ShiftedPackets * SamplesPerPacket);
    packet_writer.set_offset((Latency + ShiftedPackets * SamplesPerPacket) * NumCh);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < ShiftedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, timestamp_large_jump) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    packet_writer.set_timestamp(Latency + MaxTsJump);
    packet_writer.set_offset((Latency + MaxTsJump) * NumCh);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    while (receiver.num_sessions() != 0) {
        frame_reader.skip_zeros(SamplesPerFrame * NumCh);
    }
}

TEST(receiver_source, timestamp_overlap) {
    enum { OverlappedSamples = SamplesPerPacket / 2 };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    packet_writer.set_timestamp(Latency - OverlappedSamples);
    packet_writer.set_offset((Latency - OverlappedSamples) * NumCh);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, timestamp_reorder) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (ssize_t np = Latency / SamplesPerPacket - 1; np >= 0; np--) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }

        packet_writer.set_offset((Latency + size_t(np) * SamplesPerPacket) * NumCh);

        packet_writer.set_timestamp(
            packet::timestamp_t(Latency + size_t(np) * SamplesPerPacket));

        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    packet_writer.set_offset(Latency * 2 * NumCh);
    packet_writer.set_timestamp(Latency * 2);

    for (size_t np = 0; np < Latency / SamplesPerPacket - 1; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, timestamp_late) {
    enum { DelayedPackets = 5 };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    packet_writer.set_timestamp(Latency + DelayedPackets * SamplesPerPacket);
    packet_writer.set_offset((Latency + DelayedPackets * SamplesPerPacket) * NumCh);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < DelayedPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
        }
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    packet_writer.set_timestamp(Latency);
    packet_writer.set_offset(Latency * NumCh);

    packet_writer.write_packets(DelayedPackets, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
    }

    frame_reader.read_samples(SamplesPerFrame * NumCh, 0);
}

TEST(receiver_source, packet_size_small) {
    enum {
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,
        ManySmallPackets = Latency / SamplesPerSmallPacket * 10
    };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerSmallPacket, SamplesPerSmallPacket,
                                ChMask);

    for (size_t nf = 0; nf < ManySmallPackets / SmallPacketsPerFrame; nf++) {
        frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        for (size_t np = 0; np < SmallPacketsPerFrame; np++) {
            packet_writer.write_packets(1, SamplesPerSmallPacket, ChMask);
        }
    }
}

TEST(receiver_source, packet_size_large) {
    enum {
        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,
        ManyLargePackets = Latency / SamplesPerLargePacket * 10
    };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerLargePacket, SamplesPerLargePacket,
                                ChMask);

    for (size_t np = 0; np < ManyLargePackets; np++) {
        for (size_t nf = 0; nf < FramesPerLargePacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }
        packet_writer.write_packets(1, SamplesPerLargePacket, ChMask);
    }
}

TEST(receiver_source, packet_size_variable) {
    enum {
        SmallPacketsPerFrame = 2,
        SamplesPerSmallPacket = SamplesPerFrame / SmallPacketsPerFrame,

        FramesPerLargePacket = 2,
        SamplesPerLargePacket = SamplesPerFrame * FramesPerLargePacket,

        SamplesPerTwoPackets = (SamplesPerSmallPacket + SamplesPerLargePacket),

        NumIterations = Latency / SamplesPerTwoPackets * 10
    };

    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    size_t available = 0;

    for (size_t ni = 0; ni < NumIterations; ni++) {
        for (; available >= Latency; available -= SamplesPerFrame) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);
        }

        packet_writer.write_packets(1, SamplesPerSmallPacket, ChMask);
        packet_writer.write_packets(1, SamplesPerLargePacket, ChMask);

        available += SamplesPerTwoPackets;
    }
}

TEST(receiver_source, corrupted_packets_new_session) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.set_corrupt(true);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    for (size_t np = 0; np < ManyPackets; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.skip_zeros(SamplesPerFrame * NumCh);

            UNSIGNED_LONGS_EQUAL(0, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, corrupted_packets_existing_session) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::FrameReader frame_reader(receiver, sample_buffer_pool);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);
    packet_writer.set_corrupt(true);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    packet_writer.set_corrupt(false);

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 0);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }

    for (size_t np = 0; np < Latency / SamplesPerPacket; np++) {
        for (size_t nf = 0; nf < FramesPerPacket; nf++) {
            frame_reader.read_samples(SamplesPerFrame * NumCh, 1);

            UNSIGNED_LONGS_EQUAL(1, receiver.num_sessions());
        }

        packet_writer.write_packets(1, SamplesPerPacket, ChMask);
    }
}

TEST(receiver_source, status) {
    ReceiverSource receiver(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                            sample_buffer_pool, allocator);

    CHECK(receiver.valid());

    ReceiverSource::EndpointSetHandle endpoint_set = add_endpoint_set(receiver);
    CHECK(endpoint_set);

    packet::IWriter* endpoint1_writer =
        add_endpoint(receiver, endpoint_set, address::Iface_AudioSource, proto1);
    CHECK(endpoint1_writer);

    test::PacketWriter packet_writer(allocator, *endpoint1_writer, rtp_composer,
                                     format_map, packet_pool, byte_buffer_pool,
                                     PayloadType, src1, dst1);

    core::Slice<audio::sample_t> samples(
        new (sample_buffer_pool) core::Buffer<audio::sample_t>(sample_buffer_pool));

    CHECK(samples);
    samples.resize(FramesPerPacket * NumCh);

    CHECK(receiver.state() == sndio::ISource::Idle);

    {
        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);
    }

    packet_writer.write_packets(Latency / SamplesPerPacket, SamplesPerPacket, ChMask);

    CHECK(receiver.state() == sndio::ISource::Playing);

    {
        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);
    }

    for (;;) {
        audio::Frame frame(samples.data(), samples.size());
        receiver.read(frame);

        if (receiver.state() == sndio::ISource::Idle) {
            break;
        }
    }
}

} // namespace pipeline
} // namespace roc
