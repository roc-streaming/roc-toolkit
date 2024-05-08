/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_core/cond.h"
#include "roc_core/heap_arena.h"
#include "roc_core/mutex.h"
#include "roc_netio/network_loop.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace netio {

namespace {

enum { MaxBufSize = 500 };

core::HeapArena arena;
core::SlabPool<core::Buffer> buffer_pool("buffer_pool", arena, MaxBufSize);
core::SlabPool<packet::Packet> packet_pool("packet_pool", arena);

UdpConfig make_receiver_config(const char* ip, int port) {
    UdpConfig config;
    CHECK(config.bind_address.set_host_port(address::Family_IPv4, ip, port));
    return config;
}

class RecordingCompleter : public INetworkTaskCompleter {
public:
    RecordingCompleter()
        : cond_(mutex_)
        , task_(NULL) {
    }

    virtual void network_task_completed(NetworkTask& task) {
        core::Mutex::Lock lock(mutex_);
        task_ = &task;
        cond_.broadcast();
    }

    NetworkTask* wait_task() {
        core::Mutex::Lock lock(mutex_);
        while (!task_) {
            cond_.wait();
        }
        return task_;
    }

private:
    core::Mutex mutex_;
    core::Cond cond_;
    NetworkTask* task_;
};

class AddRemoveCompleter : public INetworkTaskCompleter {
public:
    AddRemoveCompleter(NetworkLoop& net_loop)
        : net_loop_(net_loop)
        , cond_(mutex_)
        , writer_(NULL)
        , add_task_(NULL)
        , recv_task_(NULL)
        , remove_task_(NULL) {
    }

    ~AddRemoveCompleter() {
        delete add_task_;
        delete recv_task_;
        delete remove_task_;
    }

    void start(UdpConfig& config, packet::IWriter& writer) {
        core::Mutex::Lock lock(mutex_);

        writer_ = &writer;
        add_task_ = new NetworkLoop::Tasks::AddUdpPort(config);
        net_loop_.schedule(*add_task_, *this);
    }

    void wait() {
        core::Mutex::Lock lock(mutex_);

        while (!remove_task_ || !remove_task_->success()) {
            cond_.wait();
        }
    }

    virtual void network_task_completed(NetworkTask& task) {
        core::Mutex::Lock lock(mutex_);

        if (&task == add_task_) {
            roc_panic_if_not(net_loop_.num_ports() == 1);

            roc_panic_if_not(add_task_->success());
            roc_panic_if_not(add_task_->get_handle());

            recv_task_ =
                new NetworkLoop::Tasks::StartUdpRecv(add_task_->get_handle(), *writer_);
            net_loop_.schedule(*recv_task_, *this);

            return;
        }

        if (&task == recv_task_) {
            roc_panic_if_not(net_loop_.num_ports() == 1);

            roc_panic_if_not(recv_task_->success());

            remove_task_ = new NetworkLoop::Tasks::RemovePort(add_task_->get_handle());
            net_loop_.schedule(*remove_task_, *this);

            return;
        }

        if (&task == remove_task_) {
            roc_panic_if_not(net_loop_.num_ports() == 0);

            roc_panic_if_not(remove_task_->success());
            cond_.signal();

            return;
        }

        roc_panic("unexpected task");
    }

private:
    NetworkLoop& net_loop_;

    core::Mutex mutex_;
    core::Cond cond_;

    packet::IWriter* writer_;

    NetworkLoop::Tasks::AddUdpPort* add_task_;
    NetworkLoop::Tasks::StartUdpRecv* recv_task_;
    NetworkLoop::Tasks::RemovePort* remove_task_;
};

} // namespace

TEST_GROUP(tasks) {};

TEST(tasks, synchronous_add) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig config = make_receiver_config("127.0.0.1", 0);

    NetworkLoop::Tasks::AddUdpPort add_task(config);

    CHECK(!add_task.success());
    CHECK(!add_task.get_handle());

    CHECK(net_loop.schedule_and_wait(add_task));

    CHECK(add_task.success());
    CHECK(add_task.get_handle());
}

TEST(tasks, synchronous_add_recv_remove) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig config = make_receiver_config("127.0.0.1", 0);
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    NetworkLoop::Tasks::AddUdpPort add_task(config);
    CHECK(!add_task.success());
    CHECK(!add_task.get_handle());

    CHECK(net_loop.schedule_and_wait(add_task));
    CHECK(add_task.success());
    CHECK(add_task.get_handle());

    NetworkLoop::Tasks::StartUdpRecv recv_task(add_task.get_handle(), queue);
    CHECK(!recv_task.success());

    CHECK(net_loop.schedule_and_wait(recv_task));
    CHECK(recv_task.success());

    NetworkLoop::Tasks::RemovePort remove_task(add_task.get_handle());
    CHECK(!remove_task.success());

    CHECK(net_loop.schedule_and_wait(remove_task));
    CHECK(remove_task.success());
}

TEST(tasks, asynchronous_add) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig config = make_receiver_config("127.0.0.1", 0);

    NetworkLoop::Tasks::AddUdpPort task(config);

    CHECK(!task.success());
    CHECK(!task.get_handle());

    RecordingCompleter completer;

    net_loop.schedule(task, completer);

    CHECK(completer.wait_task() == &task);

    CHECK(task.success());
    CHECK(task.get_handle());
}

TEST(tasks, asynchronous_add_recv_remove) {
    NetworkLoop net_loop(packet_pool, buffer_pool, arena);
    LONGS_EQUAL(status::StatusOK, net_loop.init_status());

    UdpConfig config = make_receiver_config("127.0.0.1", 0);
    packet::ConcurrentQueue queue(packet::ConcurrentQueue::Blocking);

    AddRemoveCompleter completer(net_loop);

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());

    completer.start(config, queue);
    completer.wait();

    UNSIGNED_LONGS_EQUAL(0, net_loop.num_ports());
}

} // namespace netio
} // namespace roc
