/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/frame_writer.h"
#include "test_helpers/packet_reader.h"
#include "test_helpers/scheduler.h"

#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_funcs.h"
#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/time.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace pipeline {

namespace {

rtp::PayloadType PayloadType = rtp::PayloadType_L16_Stereo;

enum {
    MaxBufSize = 1000,

    SampleRate = 44100,
    ChMask = 0x3,
    NumCh = 2,

    SamplesPerFrame = 20,
    SamplesPerPacket = 100,
    FramesPerPacket = SamplesPerPacket / SamplesPerFrame,

    ManyFrames = FramesPerPacket * 20
};

const core::nanoseconds_t MaxBufDuration =
    MaxBufSize * core::Second / (SampleRate * packet::num_channels(ChMask));

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxBufSize, true);
core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

rtp::FormatMap format_map;
rtp::Parser rtp_parser(format_map, NULL);

SenderSink::EndpointSetHandle add_endpoint_set(SenderSink& sender) {
    pipeline::SenderSink::Tasks::AddEndpointSet task;
    CHECK(sender.schedule_and_wait(task));

    CHECK(task.success());
    CHECK(task.get_handle());

    return task.get_handle();
}

SenderSink::EndpointHandle create_endpoint(SenderSink& sender,
                                           SenderSink::EndpointSetHandle endpoint_set,
                                           address::Interface iface,
                                           address::Protocol proto) {
    pipeline::SenderSink::Tasks::CreateEndpoint task(endpoint_set, iface, proto);
    CHECK(sender.schedule_and_wait(task));

    CHECK(task.success());
    CHECK(task.get_handle());

    return task.get_handle();
}

void set_endpoint_output_writer(SenderSink& sender,
                                SenderSink::EndpointHandle endpoint,
                                packet::IWriter& writer) {
    pipeline::SenderSink::Tasks::SetEndpointOutputWriter task(endpoint, writer);
    CHECK(sender.schedule_and_wait(task));

    CHECK(task.success());
}

void set_endpoint_destination_udp_address(SenderSink& sender,
                                          SenderSink::EndpointHandle endpoint,
                                          const address::SocketAddr& addr) {
    pipeline::SenderSink::Tasks::SetEndpointDestinationUdpAddress task(endpoint, addr);
    CHECK(sender.schedule_and_wait(task));

    CHECK(task.success());
}

class TaskIssuer : public TaskPipeline::ICompletionHandler {
public:
    TaskIssuer(TaskPipeline& pipeline)
        : pipeline_(pipeline)
        , endpoint_set_(NULL)
        , task_add_endpoint_set_(NULL)
        , task_create_endpoint_(NULL)
        , done_(false) {
    }

    ~TaskIssuer() {
        delete task_add_endpoint_set_;
        delete task_create_endpoint_;
    }

    void start() {
        task_add_endpoint_set_ = new SenderSink::Tasks::AddEndpointSet();
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
            task_create_endpoint_ = new SenderSink::Tasks::CreateEndpoint(
                endpoint_set_, address::Iface_AudioSource, address::Proto_RTP);
            pipeline_.schedule(*task_create_endpoint_, *this);
            return;
        }

        if (&task == task_create_endpoint_) {
            roc_panic_if_not(task_create_endpoint_->get_handle());
            done_ = true;
            return;
        }

        roc_panic("unexpected task");
    }

private:
    TaskPipeline& pipeline_;

    SenderSink::EndpointSetHandle endpoint_set_;

    SenderSink::Tasks::AddEndpointSet* task_add_endpoint_set_;
    SenderSink::Tasks::CreateEndpoint* task_create_endpoint_;

    core::Atomic<int> done_;
};

} // namespace

TEST_GROUP(sender_sink) {
    test::Scheduler scheduler;

    SenderConfig config;

    address::Protocol source_proto;
    address::SocketAddr dst_addr;

    void setup() {
        config.input_channels = ChMask;
        config.packet_length = SamplesPerPacket * core::Second / SampleRate;
        config.internal_frame_length = MaxBufDuration;

        config.interleaving = false;
        config.timing = false;
        config.poisoning = true;
        config.profiling = true;

        source_proto = address::Proto_RTP;
        dst_addr = test::new_address(123);
    }
};

TEST(sender_sink, endpoints_sync) {
    SenderSink sender(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);
    CHECK(sender.valid());

    SenderSink::EndpointSetHandle endpoint_set = NULL;

    {
        SenderSink::Tasks::AddEndpointSet task;
        CHECK(sender.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_handle());

        endpoint_set = task.get_handle();
    }

    {
        SenderSink::Tasks::CreateEndpoint task(endpoint_set, address::Iface_AudioSource,
                                               address::Proto_RTP);
        CHECK(sender.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_handle());
    }
}

TEST(sender_sink, endpoints_async) {
    SenderSink sender(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);
    CHECK(sender.valid());

    TaskIssuer ti(sender);

    ti.start();
    ti.wait_done();

    scheduler.wait_done();
}

TEST(sender_sink, write) {
    packet::Queue queue;

    SenderSink sender(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);
    CHECK(sender.valid());

    SenderSink::EndpointSetHandle endpoint_set = add_endpoint_set(sender);
    CHECK(endpoint_set);

    SenderSink::EndpointHandle source_endpoint =
        create_endpoint(sender, endpoint_set, address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    set_endpoint_output_writer(sender, source_endpoint, queue);
    set_endpoint_destination_udp_address(sender, source_endpoint, dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyFrames; nf++) {
        frame_writer.write_samples(SamplesPerFrame * NumCh);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_pool, PayloadType, dst_addr);

    for (size_t np = 0; np < ManyFrames / FramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

TEST(sender_sink, frame_size_small) {
    enum {
        SamplesPerSmallFrame = SamplesPerFrame / 2,
        SmallFramesPerPacket = SamplesPerPacket / SamplesPerSmallFrame,
        ManySmallFrames = SmallFramesPerPacket * 20
    };

    packet::Queue queue;

    SenderSink sender(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);
    CHECK(sender.valid());

    SenderSink::EndpointSetHandle endpoint_set = add_endpoint_set(sender);
    CHECK(endpoint_set);

    SenderSink::EndpointHandle source_endpoint =
        create_endpoint(sender, endpoint_set, address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    set_endpoint_output_writer(sender, source_endpoint, queue);
    set_endpoint_destination_udp_address(sender, source_endpoint, dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManySmallFrames; nf++) {
        frame_writer.write_samples(SamplesPerSmallFrame * NumCh);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_pool, PayloadType, dst_addr);

    for (size_t np = 0; np < ManySmallFrames / SmallFramesPerPacket; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

TEST(sender_sink, frame_size_large) {
    enum {
        SamplesPerLargeFrame = SamplesPerPacket * 4,
        PacketsPerLargeFrame = SamplesPerLargeFrame / SamplesPerPacket,
        ManyLargeFrames = 20
    };

    packet::Queue queue;

    SenderSink sender(scheduler, config, format_map, packet_pool, byte_buffer_pool,
                      sample_buffer_pool, allocator);
    CHECK(sender.valid());

    SenderSink::EndpointSetHandle endpoint_set = add_endpoint_set(sender);
    CHECK(endpoint_set);

    SenderSink::EndpointHandle source_endpoint =
        create_endpoint(sender, endpoint_set, address::Iface_AudioSource, source_proto);
    CHECK(source_endpoint);

    set_endpoint_output_writer(sender, source_endpoint, queue);
    set_endpoint_destination_udp_address(sender, source_endpoint, dst_addr);

    test::FrameWriter frame_writer(sender, sample_buffer_pool);

    for (size_t nf = 0; nf < ManyLargeFrames; nf++) {
        frame_writer.write_samples(SamplesPerLargeFrame * NumCh);
    }

    test::PacketReader packet_reader(allocator, queue, rtp_parser, format_map,
                                     packet_pool, PayloadType, dst_addr);

    for (size_t np = 0; np < ManyLargeFrames * PacketsPerLargeFrame; np++) {
        packet_reader.read_packet(SamplesPerPacket, ChMask);
    }

    CHECK(!queue.read());
}

} // namespace pipeline
} // namespace roc
