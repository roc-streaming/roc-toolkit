/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/heap_arena.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/thread.h"
#include "roc_core/time.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_status/status_code.h"

namespace roc {
namespace packet {

namespace {

enum { MaxBufSize = 100 };

core::HeapArena arena;
PacketFactory packet_factory(arena, MaxBufSize);

PacketPtr new_packet() {
    PacketPtr packet = packet_factory.new_packet();
    CHECK(packet);
    return packet;
}

struct TestWriter : core::Thread {
    TestWriter(ConcurrentQueue& queue, PacketPtr packet)
        : queue(queue)
        , packet(packet) {
    }

    ConcurrentQueue& queue;
    PacketPtr packet;

    virtual void run() {
        core::sleep_for(core::ClockMonotonic, core::Microsecond * 10);
        LONGS_EQUAL(status::StatusOK, queue.write(packet));
    }
};

} // namespace

TEST_GROUP(concurrent_queue) {};

TEST(concurrent_queue, blocking_queue_write_one_read_one) {
    ConcurrentQueue queue(ConcurrentQueue::Blocking);

    for (size_t i = 0; i < 100; i++) {
        PacketPtr wp = new_packet();
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
        CHECK(wp == rp);
    }
}

TEST(concurrent_queue, blocking_queue_write_many_read_many) {
    ConcurrentQueue queue(ConcurrentQueue::Blocking);

    for (size_t i = 0; i < 100; i++) {
        PacketPtr packets[10];

        for (size_t j = 0; j < ROC_ARRAY_SIZE(packets); j++) {
            packets[j] = new_packet();
        }

        for (size_t j = 0; j < ROC_ARRAY_SIZE(packets); j++) {
            LONGS_EQUAL(status::StatusOK, queue.write(packets[j]));
        }

        for (size_t j = 0; j < ROC_ARRAY_SIZE(packets); j++) {
            PacketPtr pp;
            LONGS_EQUAL(status::StatusOK, queue.read(pp, ModeFetch));
            CHECK(pp == packets[j]);
        }
    }
}

TEST(concurrent_queue, blocking_queue_read_empty) {
    ConcurrentQueue queue(ConcurrentQueue::Blocking);

    for (size_t i = 0; i < 100; i++) {
        PacketPtr wp = new_packet();

        TestWriter writer(queue, wp);
        CHECK(writer.start());
        writer.join();

        PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
        CHECK(wp == rp);
    }
}

TEST(concurrent_queue, blocking_queue_fetch_peek) {
    ConcurrentQueue queue(ConcurrentQueue::Blocking);

    for (size_t i = 0; i < 100; i++) {
        PacketPtr wp = new_packet();
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        for (size_t j = 0; j < 5; j++) {
            PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, queue.read(rp, ModePeek));
            CHECK(wp == rp);
        }

        PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
        CHECK(wp == rp);
    }
}

TEST(concurrent_queue, nonblocking_queue_write_one_read_one) {
    ConcurrentQueue queue(ConcurrentQueue::NonBlocking);

    for (size_t i = 0; i < 100; i++) {
        PacketPtr wp = new_packet();
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
        CHECK(wp == rp);
    }
}

TEST(concurrent_queue, nonblocking_queue_write_many_read_many) {
    ConcurrentQueue queue(ConcurrentQueue::NonBlocking);

    for (size_t i = 0; i < 100; i++) {
        PacketPtr packets[10];

        for (size_t j = 0; j < ROC_ARRAY_SIZE(packets); j++) {
            packets[j] = new_packet();
        }

        for (size_t j = 0; j < ROC_ARRAY_SIZE(packets); j++) {
            LONGS_EQUAL(status::StatusOK, queue.write(packets[j]));
        }

        for (size_t j = 0; j < ROC_ARRAY_SIZE(packets); j++) {
            PacketPtr pp;
            LONGS_EQUAL(status::StatusOK, queue.read(pp, ModeFetch));
            CHECK(pp == packets[j]);
        }
    }
}

TEST(concurrent_queue, nonblocking_queue_read_empty) {
    ConcurrentQueue queue(ConcurrentQueue::NonBlocking);

    for (size_t i = 0; i < 100; i++) {
        PacketPtr wp = new_packet();
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
        CHECK(wp == rp);

        PacketPtr pp;
        LONGS_EQUAL(status::StatusDrain, queue.read(pp, ModeFetch));
        CHECK(!pp);
    }
}

TEST(concurrent_queue, nonblocking_queue_fetch_peek) {
    ConcurrentQueue queue(ConcurrentQueue::NonBlocking);

    for (size_t i = 0; i < 100; i++) {
        PacketPtr wp = new_packet();
        LONGS_EQUAL(status::StatusOK, queue.write(wp));

        for (size_t j = 0; j < 5; j++) {
            PacketPtr rp;
            LONGS_EQUAL(status::StatusOK, queue.read(rp, ModePeek));
            CHECK(wp == rp);
        }

        PacketPtr rp;
        LONGS_EQUAL(status::StatusOK, queue.read(rp, ModeFetch));
        CHECK(wp == rp);

        PacketPtr pp;
        LONGS_EQUAL(status::StatusDrain, queue.read(pp, ModeFetch));
        CHECK(!pp);
    }
}

} // namespace packet
} // namespace roc
