/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_rtcp/composer.h"
#include "roc_rtcp/headers.h"
#include "roc_rtcp/ireceiver_hooks.h"
#include "roc_rtcp/isender_hooks.h"
#include "roc_rtcp/session.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtcp {

namespace {

class StatusWriter : public packet::IWriter, public core::NonCopyable<> {
public:
    explicit StatusWriter(status::StatusCode code)
        : call_count(0)
        , code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr&) {
        ++call_count;
        return code_;
    }

    unsigned call_count;

private:
    status::StatusCode code_;
};

struct TestReceiverHooks : public IReceiverHooks, public core::NonCopyable<> {
    virtual void on_update_source(packet::stream_source_t, const char*) {
    }

    virtual void on_remove_source(packet::stream_source_t) {
    }

    virtual size_t on_get_num_sources() {
        return 0;
    }

    virtual ReceptionMetrics on_get_reception_metrics(size_t) {
        ReceptionMetrics metrics;
        return metrics;
    }

    virtual void on_add_sending_metrics(const SendingMetrics& metrics) {
        sending_metrics = metrics;
    }

    virtual void on_add_link_metrics(const LinkMetrics&) {
    }

    SendingMetrics sending_metrics;
};

struct TestSenderHooks : public ISenderHooks, public core::NonCopyable<> {
    virtual size_t on_get_num_sources() {
        return 0;
    }

    virtual packet::stream_source_t on_get_sending_source(size_t) {
        return packet::stream_source_t(0);
    }

    virtual SendingMetrics on_get_sending_metrics(core::nanoseconds_t report_time) {
        SendingMetrics metrics;
        metrics.origin_time = report_time;

        return metrics;
    }

    virtual void on_add_reception_metrics(const ReceptionMetrics&) {
    }

    virtual void on_add_link_metrics(const LinkMetrics&) {
    }
};

} // namespace

TEST_GROUP(session) {};

TEST(session, write_packet) {
    enum { PacketSz = 128 };

    TestReceiverHooks receiver_hooks;
    TestSenderHooks sender_hooks;
    Composer composer;
    packet::Queue queue;

    core::HeapArena arena;
    packet::PacketFactory packet_factory(arena);
    core::BufferFactory<uint8_t> buffer_factory(arena, PacketSz);

    Session session(&receiver_hooks, &sender_hooks, &queue, composer, packet_factory,
                    buffer_factory);
    CHECK(session.is_valid());

    const core::nanoseconds_t now = core::Second;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, session.generate_packets(now));

    packet::PacketPtr pp;
    UNSIGNED_LONGS_EQUAL(status::StatusOK, queue.read(pp));
    CHECK(pp);

    UNSIGNED_LONGS_EQUAL(0, receiver_hooks.sending_metrics.origin_time);
    UNSIGNED_LONGS_EQUAL(status::StatusOK, session.process_packet(pp));
    UNSIGNED_LONGS_EQUAL(now, receiver_hooks.sending_metrics.origin_time);
}

TEST(session, failed_to_write_packet) {
    enum { PacketSz = 1 };
    CHECK(PacketSz < sizeof(header::PacketHeader));

    TestReceiverHooks receiver_hooks;
    TestSenderHooks sender_hooks;
    Composer composer;
    StatusWriter writer(status::StatusOK);

    core::HeapArena arena;
    packet::PacketFactory packet_factory(arena);
    core::BufferFactory<uint8_t> buffer_factory(arena, PacketSz);

    Session session(&receiver_hooks, &sender_hooks, &writer, composer, packet_factory,
                    buffer_factory);
    CHECK(session.is_valid());

    packet::PacketPtr pp = packet_factory.new_packet();
    pp->add_flags(packet::Packet::FlagRTCP);

    core::Slice<uint8_t> buf = buffer_factory.new_buffer();
    pp->rtcp()->data = buf;

    // TODO(gh-183): compare with status::StatusBadArg
    UNSIGNED_LONGS_EQUAL(0, receiver_hooks.sending_metrics.origin_time);
    UNSIGNED_LONGS_EQUAL(status::StatusOK, session.process_packet(pp));
    UNSIGNED_LONGS_EQUAL(0, receiver_hooks.sending_metrics.origin_time);
}

TEST(session, generate_packets_failed_to_write) {
    enum { PacketSz = 128 };

    const status::StatusCode failure_status_code = status::StatusNoData;

    TestReceiverHooks receiver_hooks;
    TestSenderHooks sender_hooks;
    Composer composer;
    StatusWriter writer(failure_status_code);

    core::HeapArena arena;
    packet::PacketFactory packet_factory(arena);
    core::BufferFactory<uint8_t> buffer_factory(arena, PacketSz);

    Session session(&receiver_hooks, &sender_hooks, &writer, composer, packet_factory,
                    buffer_factory);
    CHECK(session.is_valid());

    core::nanoseconds_t now = core::Second;

    // Failed to write.
    UNSIGNED_LONGS_EQUAL(failure_status_code, session.generate_packets(now));
    UNSIGNED_LONGS_EQUAL(1, writer.call_count);

    // It's too early, wait until next deadline.
    UNSIGNED_LONGS_EQUAL(status::StatusOK, session.generate_packets(now));
    UNSIGNED_LONGS_EQUAL(1, writer.call_count);

    now += core::Second;

    // Failed to write.
    UNSIGNED_LONGS_EQUAL(failure_status_code, session.generate_packets(now));
    UNSIGNED_LONGS_EQUAL(2, writer.call_count);
}

} // namespace rtcp
} // namespace roc
