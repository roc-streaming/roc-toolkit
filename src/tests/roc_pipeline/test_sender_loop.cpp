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
#include "roc_packet/fifo_queue.h"
#include "roc_pipeline/sender_loop.h"
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
        SenderSlotConfig slot_config;
        task_create_slot_ = new SenderLoop::Tasks::CreateSlot(slot_config);
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
            task_add_endpoint_ = new SenderLoop::Tasks::AddEndpoint(
                slot_, address::Iface_AudioSource, address::Proto_RTP, outbound_address_,
                outbound_writer_);
            pipeline_.schedule(*task_add_endpoint_, *this);
            return;
        }

        if (&task == task_add_endpoint_) {
            roc_panic_if(task_add_endpoint_->get_inbound_writer());
            task_delete_slot_ = new SenderLoop::Tasks::DeleteSlot(slot_);
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

    SenderLoop::SlotHandle slot_;

    address::SocketAddr outbound_address_;
    packet::FifoQueue outbound_writer_;

    SenderLoop::Tasks::CreateSlot* task_create_slot_;
    SenderLoop::Tasks::AddEndpoint* task_add_endpoint_;
    SenderLoop::Tasks::DeleteSlot* task_delete_slot_;

    core::Atomic<int> done_;
};

} // namespace

TEST_GROUP(sender_loop) {
    test::MockScheduler scheduler;

    SenderSinkConfig config;

    void setup() {
        config.latency.tuner_backend = audio::LatencyTunerBackend_Niq;
        config.latency.tuner_profile = audio::LatencyTunerProfile_Intact;
    }
};

TEST(sender_loop, endpoints_sync) {
    SenderLoop sender(scheduler, config, processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    SenderLoop::SlotHandle slot = NULL;

    address::SocketAddr outbound_address;
    packet::FifoQueue outbound_writer;

    {
        SenderSlotConfig config;
        SenderLoop::Tasks::CreateSlot task(config);
        CHECK(sender.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_handle());

        slot = task.get_handle();
    }

    {
        SenderLoop::Tasks::AddEndpoint task(slot, address::Iface_AudioSource,
                                            address::Proto_RTP, outbound_address,
                                            outbound_writer);
        CHECK(sender.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(!task.get_inbound_writer());
    }

    {
        SenderLoop::Tasks::DeleteSlot task(slot);
        CHECK(sender.schedule_and_wait(task));
        CHECK(task.success());
    }
}

TEST(sender_loop, endpoints_async) {
    SenderLoop sender(scheduler, config, processor_map, encoding_map, packet_pool,
                      packet_buffer_pool, frame_pool, frame_buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, sender.init_status());

    TaskIssuer ti(sender);

    ti.start();
    ti.wait_done();

    scheduler.wait_done();
}

} // namespace pipeline
} // namespace roc
