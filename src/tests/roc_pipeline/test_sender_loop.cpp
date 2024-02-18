/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_scheduler.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/sender_loop.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace pipeline {

namespace {

enum { MaxBufSize = 1000 };

core::HeapArena arena;
core::BufferFactory<audio::sample_t> sample_buffer_factory(arena, MaxBufSize);
core::BufferFactory<uint8_t> byte_buffer_factory(arena, MaxBufSize);
packet::PacketFactory packet_factory(arena);

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
    packet::Queue outbound_writer_;

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
    SenderLoop sender(scheduler, config, encoding_map, packet_factory,
                      byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    SenderLoop::SlotHandle slot = NULL;

    address::SocketAddr outbound_address;
    packet::Queue outbound_writer;

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
    SenderLoop sender(scheduler, config, encoding_map, packet_factory,
                      byte_buffer_factory, sample_buffer_factory, arena);
    CHECK(sender.is_valid());

    TaskIssuer ti(sender);

    ti.start();
    ti.wait_done();

    scheduler.wait_done();
}

} // namespace pipeline
} // namespace roc
