/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/cond.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/mutex.h"
#include "roc_netio/event_loop.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapAllocator allocator;
core::BufferPool<uint8_t> buffer_pool(allocator, MaxBufSize, true);
packet::PacketPool packet_pool(allocator, true);

UdpReceiverConfig make_receiver_config(const char* ip, int port) {
    UdpReceiverConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port));
    return config;
}

core::Mutex cb_mutex;
core::Cond cb_cond(cb_mutex);

bool cb_invoked;

void* recorded_cb_arg;
EventLoop::Task* recorded_task;

void recording_callback(void* cb_arg, EventLoop::Task& task) {
    CHECK(cb_arg);

    CHECK(task.success());
    CHECK(((EventLoop::Tasks::AddUdpReceiverPort&)task).get_handle());

    core::Mutex::Lock lock(cb_mutex);

    recorded_cb_arg = cb_arg;
    recorded_task = &task;

    cb_invoked = true;
    cb_cond.broadcast();
}

void removed_callback(void* cb_arg, EventLoop::Task& task) {
    EventLoop& event_loop = *(EventLoop*)cb_arg;
    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());

    EventLoop::Tasks::RemovePort* remove_task = (EventLoop::Tasks::RemovePort*)&task;

    delete remove_task;

    core::Mutex::Lock lock(cb_mutex);

    cb_invoked = true;
    cb_cond.broadcast();
}

void added_callback(void* cb_arg, EventLoop::Task& task) {
    EventLoop& event_loop = *(EventLoop*)cb_arg;
    UNSIGNED_LONGS_EQUAL(1, event_loop.num_ports());

    EventLoop::Tasks::AddUdpReceiverPort& add_task =
        (EventLoop::Tasks::AddUdpReceiverPort&)task;

    CHECK(add_task.success());
    CHECK(add_task.get_handle());

    EventLoop::Tasks::RemovePort* remove_task =
        new EventLoop::Tasks::RemovePort(add_task.get_handle());

    event_loop.enqueue(*remove_task, removed_callback, &event_loop);
}

} // namespace

TEST_GROUP(tasks) {};

TEST(tasks, synchronous_add) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpReceiverConfig config = make_receiver_config("127.0.0.1", 0);
    packet::ConcurrentQueue queue;

    EventLoop::Tasks::AddUdpReceiverPort task(config, queue);

    CHECK(!task.success());
    CHECK(!task.get_handle());

    CHECK(event_loop.enqueue_and_wait(task));

    CHECK(task.success());
    CHECK(task.get_handle());
}

TEST(tasks, asynchronous_add) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpReceiverConfig config = make_receiver_config("127.0.0.1", 0);
    packet::ConcurrentQueue queue;
    char cb_arg;

    EventLoop::Tasks::AddUdpReceiverPort task(config, queue);

    CHECK(!task.success());
    CHECK(!task.get_handle());

    recorded_cb_arg = NULL;
    recorded_task = NULL;

    cb_invoked = false;

    {
        core::Mutex::Lock lock(cb_mutex);

        event_loop.enqueue(task, recording_callback, &cb_arg);

        while (!cb_invoked) {
            cb_cond.wait();
        }
    }

    CHECK(recorded_cb_arg == &cb_arg);
    CHECK(recorded_task == &task);

    CHECK(task.success());
    CHECK(task.get_handle());
}

TEST(tasks, asynchronous_add_remove) {
    EventLoop event_loop(packet_pool, buffer_pool, allocator);
    CHECK(event_loop.valid());

    UdpReceiverConfig config = make_receiver_config("127.0.0.1", 0);
    packet::ConcurrentQueue queue;

    EventLoop::Tasks::AddUdpReceiverPort task(config, queue);

    cb_invoked = false;

    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());

    {
        core::Mutex::Lock lock(cb_mutex);

        event_loop.enqueue(task, added_callback, &event_loop);

        while (!cb_invoked) {
            cb_cond.wait();
        }
    }

    UNSIGNED_LONGS_EQUAL(0, event_loop.num_ports());
}

} // namespace netio
} // namespace roc
