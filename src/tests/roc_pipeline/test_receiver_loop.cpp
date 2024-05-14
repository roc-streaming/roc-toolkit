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
#include "roc_fec/codec_map.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/receiver_loop.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace pipeline {

namespace {

enum { MaxBufSize = 1000 };

core::HeapArena arena;
core::BufferFactory sample_buffer_factory(arena, MaxBufSize * sizeof(audio::sample_t));
core::BufferFactory byte_buffer_factory(arena, MaxBufSize);
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
    }
};

TEST(receiver_loop, endpoints_sync) {
    ReceiverLoop receiver(scheduler, config, encoding_map, packet_factory,
                          byte_buffer_factory, sample_buffer_factory, arena);

    CHECK(receiver.is_valid());

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
    ReceiverLoop receiver(scheduler, config, encoding_map, packet_factory,
                          byte_buffer_factory, sample_buffer_factory, arena);

    CHECK(receiver.is_valid());

    TaskIssuer ti(receiver);

    ti.start();
    ti.wait_done();

    scheduler.wait_done();
}

} // namespace pipeline
} // namespace roc
