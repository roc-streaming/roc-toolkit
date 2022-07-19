/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/scheduler.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_allocator.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/sender_loop.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

namespace {

enum { MaxBufSize = 1000 };

core::HeapAllocator allocator;
core::BufferFactory<audio::sample_t> sample_buffer_factory(allocator, MaxBufSize, true);
core::BufferFactory<uint8_t> byte_buffer_factory(allocator, MaxBufSize, true);
packet::PacketFactory packet_factory(allocator, true);

rtp::FormatMap format_map;

class TaskIssuer : public IPipelineTaskCompleter {
public:
    TaskIssuer(PipelineLoop& pipeline)
        : pipeline_(pipeline)
        , endpoint_set_(NULL)
        , task_create_endpoint_set_(NULL)
        , task_create_endpoint_(NULL)
        , done_(false) {
    }

    ~TaskIssuer() {
        delete task_create_endpoint_set_;
        delete task_create_endpoint_;
    }

    void start() {
        task_create_endpoint_set_ = new SenderLoop::Tasks::CreateEndpointSet();
        pipeline_.schedule(*task_create_endpoint_set_, *this);
    }

    void wait_done() const {
        while (!done_) {
            core::sleep_for(core::ClockMonotonic, core::Microsecond * 10);
        }
    }

    virtual void pipeline_task_completed(PipelineTask& task) {
        roc_panic_if_not(task.success());

        if (&task == task_create_endpoint_set_) {
            endpoint_set_ = task_create_endpoint_set_->get_handle();
            roc_panic_if_not(endpoint_set_);
            task_create_endpoint_ = new SenderLoop::Tasks::CreateEndpoint(
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
    PipelineLoop& pipeline_;

    SenderLoop::EndpointSetHandle endpoint_set_;

    SenderLoop::Tasks::CreateEndpointSet* task_create_endpoint_set_;
    SenderLoop::Tasks::CreateEndpoint* task_create_endpoint_;

    core::Atomic<int> done_;
};

} // namespace

TEST_GROUP(sender_loop) {
    test::Scheduler scheduler;

    SenderConfig config;
};

TEST(sender_loop, endpoints_sync) {
    SenderLoop sender(scheduler, config, format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.valid());

    SenderLoop::EndpointSetHandle endpoint_set = NULL;

    {
        SenderLoop::Tasks::CreateEndpointSet task;
        CHECK(sender.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_handle());

        endpoint_set = task.get_handle();
    }

    {
        SenderLoop::Tasks::CreateEndpoint task(endpoint_set, address::Iface_AudioSource,
                                               address::Proto_RTP);
        CHECK(sender.schedule_and_wait(task));
        CHECK(task.success());
        CHECK(task.get_handle());
    }
}

TEST(sender_loop, endpoints_async) {
    SenderLoop sender(scheduler, config, format_map, packet_factory, byte_buffer_factory,
                      sample_buffer_factory, allocator);
    CHECK(sender.valid());

    TaskIssuer ti(sender);

    ti.start();
    ti.wait_done();

    scheduler.wait_done();
}

} // namespace pipeline
} // namespace roc
