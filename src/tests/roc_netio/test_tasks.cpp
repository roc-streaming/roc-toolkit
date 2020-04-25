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
#include "roc_netio/network_loop.h"
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
NetworkLoop::Task* recorded_task;

void recording_callback(void* cb_arg, NetworkLoop::Task& task) {
    CHECK(cb_arg);

    CHECK(task.success());
    CHECK(((NetworkLoop::Tasks::AddUdpReceiverPort&)task).get_handle());

    core::Mutex::Lock lock(cb_mutex);

    recorded_cb_arg = cb_arg;
    recorded_task = &task;

    cb_invoked = true;
    cb_cond.broadcast();
}

void removed_callback(void* cb_arg, NetworkLoop::Task& task) {
    NetworkLoop& net_loop = *(NetworkLoop*)cb_arg;
    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    NetworkLoop::Tasks::RemovePort* remove_task = (NetworkLoop::Tasks::RemovePort*)&task;

    delete remove_task;

    core::Mutex::Lock lock(cb_mutex);

    cb_invoked = true;
    cb_cond.broadcast();
}

void added_callback(void* cb_arg, NetworkLoop::Task& task) {
    NetworkLoop& net_loop = *(NetworkLoop*)cb_arg;
    UNSIGNED_LONGS_EQUAL(1, net_loop.num_ports());

    NetworkLoop::Tasks::AddUdpReceiverPort& add_task =
        (NetworkLoop::Tasks::AddUdpReceiverPort&)task;

    CHECK(add_task.success());
    CHECK(add_task.get_handle());

    NetworkLoop::Tasks::RemovePort* remove_task =
        new NetworkLoop::Tasks::RemovePort(add_task.get_handle());

    net_loop.schedule(*remove_task, removed_callback, &net_loop);
}

} // namespace

TEST_GROUP(tasks) {};

TEST(tasks, synchronous_add) {
    NetworkLoop net_loop(packet_pool, buffer_pool, allocator);
    CHECK(net_loop.valid());

    UdpReceiverConfig config = make_receiver_config("127.0.0.1", 0);
    packet::ConcurrentQueue queue;

    NetworkLoop::Tasks::AddUdpReceiverPort task(config, queue);

    CHECK(!task.success());
    CHECK(!task.get_handle());

    CHECK(net_loop.schedule_and_wait(task));

    CHECK(task.success());
    CHECK(task.get_handle());
}

TEST(tasks, asynchronous_add) {
    NetworkLoop net_loop(packet_pool, buffer_pool, allocator);
    CHECK(net_loop.valid());

    UdpReceiverConfig config = make_receiver_config("127.0.0.1", 0);
    packet::ConcurrentQueue queue;
    char cb_arg;

    NetworkLoop::Tasks::AddUdpReceiverPort task(config, queue);

    CHECK(!task.success());
    CHECK(!task.get_handle());

    recorded_cb_arg = NULL;
    recorded_task = NULL;

    cb_invoked = false;

    {
        core::Mutex::Lock lock(cb_mutex);

        net_loop.schedule(task, recording_callback, &cb_arg);

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
    NetworkLoop net_loop(packet_pool, buffer_pool, allocator);
    CHECK(net_loop.valid());

    UdpReceiverConfig config = make_receiver_config("127.0.0.1", 0);
    packet::ConcurrentQueue queue;

    NetworkLoop::Tasks::AddUdpReceiverPort task(config, queue);

    cb_invoked = false;

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    {
        core::Mutex::Lock lock(cb_mutex);

        net_loop.schedule(task, added_callback, &net_loop);

        while (!cb_invoked) {
            cb_cond.wait();
        }
    }

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

} // namespace netio
} // namespace roc
