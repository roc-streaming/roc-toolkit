/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_scheduler.h"

#include "roc_core/heap_arena.h"
#include "roc_core/slab_pool.h"
#include "roc_fec/codec_map.h"
#include "roc_pipeline/receiver_loop.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace pipeline {

namespace {

enum { MaxBufSize = 1000 };

core::HeapArena arena;

core::SlabPool<packet::Packet> packet_pool("packet_pool", arena);
core::SlabPool<core::Buffer>
    packet_buffer_pool("packet_buffer_pool", arena, sizeof(core::Buffer) + MaxBufSize);

core::SlabPool<audio::Frame> frame_pool("frame_pool", arena);
core::SlabPool<core::Buffer>
    frame_buffer_pool("frame_buffer_pool",
                      arena,
                      sizeof(core::Buffer) + MaxBufSize * sizeof(audio::sample_t));

packet::PacketFactory packet_factory(packet_pool, packet_buffer_pool);
audio::FrameFactory frame_factory(frame_pool, frame_buffer_pool);

audio::ProcessorMap processor_map(arena);
rtp::EncodingMap encoding_map(arena);

class TaskIssuer : public IPipelineTaskCompleter {
public:
    TaskIssuer(PipelineLoop& pipeline)
        : pipeline_(pipeline)
        , slot_(NULL)
        , task_create_slot_(NULL)
        , task_add_endpoint_(NULL)
        , task_delete_slot_(NULL)
        , done_(false) {
    }

    ~TaskIssuer() {
        delete task_create_slot_;
        delete task_add_endpoint_;
        delete task_delete_slot_;
    }

    void start() {
        ReceiverSlotConfig slot_config;
        task_create_slot_ = new ReceiverLoop::Tasks::CreateSlot(slot_config);
        pipeline_.schedule(*task_create_slot_, *this);
    }

    void wait_done() const {
        while (!done_) {
            core::sleep_for(core::ClockMonotonic, core::Microsecond * 10);
        }
    }

    virtual void pipeline_task_completed(PipelineTask& task) {
        roc_panic_if_not(task.success());

        if (&task == task_create_slot_) {
            slot_ = task_create_slot_->get_handle();
            roc_panic_if_not(slot_);
            task_add_endpoint_ = new ReceiverLoop::Tasks::AddEndpoint(
                slot_, address::Iface_AudioSource, address::Proto_RTP,
                address::SocketAddr(), NULL);
            pipeline_.schedule(*task_add_endpoint_, *this);
            return;
        }

        if (&task == task_add_endpoint_) {
            task_delete_slot_ = new ReceiverLoop::Tasks::DeleteSlot(slot_);
            pipeline_.schedule(*task_delete_slot_, *this);
            return;
        }

        if (&task == task_delete_slot_) {
            done_ = true;
            return;
        }

        roc_panic("unexpected task");
    }

private:
    PipelineLoop& pipeline_;

    ReceiverLoop::SlotHandle slot_;

    ReceiverLoop::Tasks::CreateSlot* task_create_slot_;
    ReceiverLoop::Tasks::AddEndpoint* task_add_endpoint_;
    ReceiverLoop::Tasks::DeleteSlot* task_delete_slot_;

    core::Atomic<int> done_;
};

} // namespace

TEST_GROUP(receiver_loop) {
    test::MockScheduler scheduler;

    ReceiverSourceConfig config;

    void setup() {
        config.session_defaults.latency.tuner_backend = audio::LatencyTunerBackend_Niq;
        config.session_defaults.latency.tuner_profile = audio::LatencyTunerProfile_Intact;
        config.session_defaults.latency.target_latency = DefaultLatency;
    }
};

TEST(receiver_loop, endpoints_sync) {
    ReceiverLoop receiver(scheduler, config, processor_map, encoding_map, packet_pool,
                          packet_buffer_pool, frame_pool, frame_buffer_pool, arena);

    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    ReceiverLoop::SlotHandle slot = NULL;

    {
        ReceiverSlotConfig config;
        ReceiverLoop::Tasks::CreateSlot task(config);
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_handle());

        slot = task.get_handle();
    }

    {
        ReceiverLoop::Tasks::AddEndpoint task(slot, address::Iface_AudioSource,
                                              address::Proto_RTP, address::SocketAddr(),
                                              NULL);
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_inbound_writer());
    }

    {
        ReceiverLoop::Tasks::DeleteSlot task(slot);
        CHECK(receiver.schedule_and_wait(task));
        CHECK(task.success());
    }
}

TEST(receiver_loop, endpoints_async) {
    ReceiverLoop receiver(scheduler, config, processor_map, encoding_map, packet_pool,
                          packet_buffer_pool, frame_pool, frame_buffer_pool, arena);

    LONGS_EQUAL(status::StatusOK, receiver.init_status());

    TaskIssuer ti(receiver);

    ti.start();
    ti.wait_done();

    scheduler.wait_done();
}

} // namespace pipeline
} // namespace roc
