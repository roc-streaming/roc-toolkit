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
#include "roc_core/log.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/queue.h"
#include "roc_packet/units.h"
#include "roc_rtcp/communicator.h"
#include "roc_rtcp/composer.h"
#include "roc_rtcp/istream_controller.h"
#include "roc_rtcp/print_packet.h"

// This file contains tests that check how rtcp::Communicator allows IStreamController
// instances (senders and receivers) to exchange RTCP reports.
//
// Tests create separate IStreamController + Communicator for every sender or receiver.
// IStreamController is implemented by a mock. Tests instruct mock to return specific
// reports when communicator queries them, as well as remember notifications that
// the mock gets from communicator.
//
// Then tests ask one communicator to generate RTCP packets, and another communicator
// to process those RTCP packets. After that, tests can check that what we got in
// notification on one side, corresponds to what we returned from query on another side.
//
// These tests don't inspect RTCP packets and assume that packet building and parsing
// is already covered by other tests.
//
// If you run tests with "-t" flag (enable tracing), tests will log each transferred
//! RTCP packet in human-readable text from.

namespace roc {
namespace rtcp {

namespace {

// Mock implementation of IStreamController
struct MockController : public IStreamController, public core::NonCopyable<> {
    MockController(const char* cname, packet::stream_source_t source_id) {
        cname_ = cname;
        source_id_ = changed_source_id_ = source_id;
        has_send_report_ = false;
        memset(has_recv_report_, 0, sizeof(has_recv_report_));
        cur_send_notification_ = num_send_notifications_ = 0;
        cur_recv_notification_ = num_recv_notifications_ = 0;
        cur_halt_notification_ = num_halt_notifications_ = 0;
        memset(halt_notifications_, 0, sizeof(halt_notifications_));
        num_ssrc_change_notifications_ = 0;
    }

    ~MockController() {
        // Every test should fetch and check all pending notifications.
        CHECK_EQUAL(0, pending_notifications());
    }

    void set_send_report(const SendReport& report) {
        has_send_report_ = true;
        send_report_ = report;
    }

    void set_recv_report(size_t index, const RecvReport& report) {
        has_recv_report_[index] = true;
        recv_report_[index] = report;
    }

    void set_changed_ssrc(packet::stream_source_t source_id) {
        changed_source_id_ = source_id;
    }

    size_t pending_notifications() const {
        return (num_send_notifications_ - cur_send_notification_)
            + (num_recv_notifications_ - cur_recv_notification_)
            + (num_halt_notifications_ - cur_halt_notification_)
            + num_ssrc_change_notifications_;
    }

    SendReport next_send_notification() {
        CHECK(cur_recv_notification_ < num_recv_notifications_);
        return recv_notifications_[cur_recv_notification_++];
    }

    RecvReport next_recv_notification() {
        CHECK(cur_send_notification_ < num_send_notifications_);
        return send_notifications_[cur_send_notification_++];
    }

    packet::stream_source_t next_halt_notification() {
        CHECK(cur_halt_notification_ < num_halt_notifications_);
        return halt_notifications_[cur_halt_notification_++];
    }

    void next_ssrc_change_notification() {
        CHECK(num_ssrc_change_notifications_ > 0);
        num_ssrc_change_notifications_--;
    }

    virtual const char* cname() {
        return cname_;
    }

    virtual packet::stream_source_t source_id() {
        return source_id_;
    }

    virtual void change_source_id() {
        source_id_ = changed_source_id_;
        num_ssrc_change_notifications_++;
    }

    virtual bool has_send_stream() {
        return has_send_report_;
    }

    virtual SendReport query_send_stream(core::nanoseconds_t report_time) {
        CHECK(has_send_report_);
        CHECK_EQUAL(report_time, send_report_.report_timestamp);
        return send_report_;
    }

    virtual void notify_send_stream(packet::stream_source_t recv_source_id,
                                    const RecvReport& recv_report) {
        CHECK_EQUAL(recv_source_id, recv_report.receiver_source_id);
        CHECK(num_send_notifications_ < MaxNotifications);
        send_notifications_[num_send_notifications_++] = recv_report;
    }

    virtual size_t num_recv_steams() {
        size_t cnt = 0;
        for (size_t i = 0; i < MaxStreams; i++) {
            if (has_recv_report_[i]) {
                cnt++;
            }
        }
        return cnt;
    }

    virtual RecvReport query_recv_stream(size_t recv_stream_index,
                                         core::nanoseconds_t report_time) {
        CHECK(has_recv_report_[recv_stream_index]);
        CHECK_EQUAL(report_time, recv_report_[recv_stream_index].report_timestamp);
        return recv_report_[recv_stream_index];
    }

    virtual void notify_recv_stream(packet::stream_source_t send_source_id,
                                    const SendReport& send_report) {
        CHECK_EQUAL(send_source_id, send_report.sender_source_id);
        CHECK(num_recv_notifications_ < MaxNotifications);
        recv_notifications_[num_recv_notifications_++] = send_report;
    }

    virtual void halt_recv_stream(packet::stream_source_t send_source_id) {
        CHECK(num_halt_notifications_ < MaxNotifications);
        halt_notifications_[num_halt_notifications_++] = send_source_id;
    }

private:
    enum { MaxStreams = 100, MaxNotifications = 100 };

    const char* cname_;

    packet::stream_source_t source_id_;
    packet::stream_source_t changed_source_id_;

    bool has_send_report_;
    SendReport send_report_;

    bool has_recv_report_[MaxStreams];
    RecvReport recv_report_[MaxStreams];

    size_t cur_send_notification_;
    size_t num_send_notifications_;
    RecvReport send_notifications_[MaxNotifications];

    size_t cur_recv_notification_;
    size_t num_recv_notifications_;
    SendReport recv_notifications_[MaxNotifications];

    size_t cur_halt_notification_;
    size_t num_halt_notifications_;
    packet::stream_source_t halt_notifications_[MaxNotifications];

    size_t num_ssrc_change_notifications_;
};

// Mock implementation of IWriter
class MockWriter : public packet::IWriter, public core::NonCopyable<> {
public:
    explicit MockWriter(status::StatusCode code)
        : call_count_(0)
        , code_(code) {
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr&) {
        ++call_count_;
        return code_;
    }

    unsigned call_count() const {
        return call_count_;
    }

private:
    unsigned call_count_;
    status::StatusCode code_;
};

// Mock implementation of IArena
class MockArena : public core::IArena, public core::NonCopyable<> {
public:
    MockArena()
        : fail_(false) {
    }

    virtual void* allocate(size_t size) {
        if (fail_) {
            return NULL;
        }
        return ha_.allocate(size);
    }

    virtual void deallocate(void* ptr) {
        ha_.deallocate(ptr);
    }

    void set_fail(bool fail) {
        fail_ = fail;
    }

private:
    core::HeapArena ha_;
    bool fail_;
};

SendReport make_send_report(core::nanoseconds_t time,
                            const char* send_cname,
                            packet::stream_source_t send_ssrc,
                            unsigned int seed) {
    SendReport report;
    report.sender_source_id = send_ssrc;
    report.sender_cname = send_cname;
    report.report_timestamp = time;
    report.stream_timestamp = seed + 10;
    report.packet_count = seed + 20;
    report.byte_count = seed + 30;
    return report;
}

void expect_send_report(const SendReport& report,
                        core::nanoseconds_t time,
                        const char* send_cname,
                        packet::stream_source_t send_ssrc,
                        unsigned int seed,
                        bool expect_xr = true) {
    CHECK_EQUAL(send_ssrc, report.sender_source_id);
    STRCMP_EQUAL(send_cname, report.sender_cname);
    CHECK(core::ns_equal_delta(time, report.report_timestamp, 10 * core::Nanosecond));
    CHECK_EQUAL(seed + 10, report.stream_timestamp);
    CHECK_EQUAL(seed + 20, report.packet_count);
    CHECK_EQUAL(seed + 30, report.byte_count);
}

RecvReport make_recv_report(core::nanoseconds_t time,
                            const char* recv_cname,
                            packet::stream_source_t recv_ssrc,
                            packet::stream_source_t send_ssrc,
                            unsigned int seed) {
    RecvReport report;
    report.receiver_cname = recv_cname;
    report.receiver_source_id = recv_ssrc;
    report.sender_source_id = send_ssrc;
    report.report_timestamp = time;
    report.extended_seqnum = seed * 100000;
    report.fract_loss = seed * 0.001f;
    report.cum_loss = (int)seed + 10;
    report.jitter = seed + 20;
    return report;
}

void expect_recv_report(const RecvReport& report,
                        core::nanoseconds_t time,
                        const char* recv_cname,
                        packet::stream_source_t recv_ssrc,
                        packet::stream_source_t send_ssrc,
                        unsigned int seed,
                        bool expect_xr = true) {
    STRCMP_EQUAL(recv_cname, report.receiver_cname);
    CHECK_EQUAL(recv_ssrc, report.receiver_source_id);
    CHECK_EQUAL(send_ssrc, report.sender_source_id);
    CHECK_EQUAL(seed * 100000, report.extended_seqnum);
    DOUBLES_EQUAL(seed * 0.001f, report.fract_loss, 0.005);
    CHECK_EQUAL((int)seed + 10, report.cum_loss);
    CHECK_EQUAL(seed + 20, report.jitter);
    if (expect_xr) {
        CHECK(core::ns_equal_delta(time, report.report_timestamp, 10 * core::Nanosecond));
    } else {
        CHECK(report.report_timestamp == 0);
    }
}

packet::PacketPtr read_packet(packet::Queue& source) {
    CHECK(source.size() != 0);
    packet::PacketPtr pp;
    CHECK_EQUAL(status::StatusOK, source.read(pp));
    CHECK(pp);
    CHECK(pp->rtcp());
    CHECK(pp->rtcp()->payload);
    roc_log(LogTrace, "delivering rtcp packet");
    if (core::Logger::instance().get_level() >= LogTrace) {
        print_packet(pp->rtcp()->payload);
    }
    return pp;
}

void advance_time(core::nanoseconds_t& time, core::nanoseconds_t delta = core::Second) {
    time += delta;
}

enum { MaxPacketSz = 200 };

core::HeapArena arena;
packet::PacketFactory packet_factory(arena);
core::BufferFactory<uint8_t> buffer_factory(arena, MaxPacketSz);
Composer composer;

} // namespace

TEST_GROUP(communicator) {};

TEST(communicator, one_sender_one_receiver) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1);

    advance_time(send_time);
    advance_time(recv_time);

    // Generate receiver report
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Deliver receiver report to sender
    CHECK_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.num_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_ctl.pending_notifications());
    expect_recv_report(send_ctl.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       SendSsrc, Seed2);
}

TEST(communicator, two_senders_one_receiver) {
    enum {
        Send1Ssrc = 11,
        Send2Ssrc = 22,
        RecvSsrc = 33,
        Seed1 = 100,
        Seed2 = 200,
        Seed3 = 300,
        Seed4 = 400,
    };

    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::Queue send1_queue;
    MockController send1_ctl(Send1Cname, Send1Ssrc);
    Communicator send1_comm(config, send1_ctl, &send1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    packet::Queue send2_queue;
    MockController send2_ctl(Send2Cname, Send2Ssrc);
    Communicator send2_comm(config, send2_ctl, &send2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send1_time = 10000000000000000;
    core::nanoseconds_t send2_time = 30000000000000000;
    core::nanoseconds_t recv_time = 60000000000000000;

    // Generate sender 1 report
    send1_ctl.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(0, send1_comm.num_streams());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender 1 report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send1_time, Send1Cname,
                       Send1Ssrc, Seed1);

    advance_time(recv_time);

    // Generate sender 2 report
    send2_ctl.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(0, send2_comm.num_streams());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender 2 report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send2_time, Send2Cname,
                       Send2Ssrc, Seed2);

    advance_time(send1_time);
    advance_time(send2_time);
    advance_time(recv_time);

    // Generate receiver report
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed3));
    recv_ctl.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed4));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(2, recv_comm.num_streams());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver receiver report to sender 1 and 2
    packet::PacketPtr pp = read_packet(recv_queue);
    CHECK_EQUAL(status::StatusOK, send1_comm.process_packet(pp, send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());
    CHECK_EQUAL(status::StatusOK, send2_comm.process_packet(pp, send2_time));
    CHECK_EQUAL(1, send2_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    expect_recv_report(send1_ctl.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       Send1Ssrc, Seed3);

    // Check notifications on sender 2
    CHECK_EQUAL(1, send2_ctl.pending_notifications());
    expect_recv_report(send2_ctl.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       Send2Ssrc, Seed4);
}

TEST(communicator, one_sender_two_receivers) {
    enum {
        SendSsrc = 11,
        Recv1Ssrc = 22,
        Recv2Ssrc = 33,
        Seed1 = 100,
        Seed2 = 200,
        Seed3 = 300
    };

    const char* SendCname = "send_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    Config config;

    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    packet::Queue recv1_queue;
    MockController recv1_ctl(Recv1Cname, Recv1Ssrc);
    Communicator recv1_comm(config, recv1_ctl, &recv1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(recv1_comm.is_valid());

    packet::Queue recv2_queue;
    MockController recv2_ctl(Recv2Cname, Recv2Ssrc);
    Communicator recv2_comm(config, recv2_ctl, &recv2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(recv2_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv1_time = 30000000000000000;
    core::nanoseconds_t recv2_time = 60000000000000000;

    // Generate sender report
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver 1 and 2
    packet::PacketPtr pp = read_packet(send_queue);
    CHECK_EQUAL(status::StatusOK, recv1_comm.process_packet(pp, recv1_time));
    CHECK_EQUAL(1, recv1_comm.num_streams());
    CHECK_EQUAL(status::StatusOK, recv2_comm.process_packet(pp, recv2_time));
    CHECK_EQUAL(1, recv2_comm.num_streams());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_ctl.pending_notifications());
    expect_send_report(recv1_ctl.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1);

    // Check notifications on receiver 2
    CHECK_EQUAL(1, recv2_ctl.pending_notifications());
    expect_send_report(recv2_ctl.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1);

    advance_time(send_time);
    advance_time(recv1_time);

    // Generate receiver 1 report
    recv1_ctl.set_recv_report(
        0, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, recv1_comm.generate_reports(recv1_time));
    CHECK_EQUAL(1, recv1_comm.num_streams());
    CHECK_EQUAL(1, recv1_queue.size());

    // Deliver receiver 1 report to sender
    CHECK_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv1_queue), send_time));
    CHECK_EQUAL(1, send_comm.num_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_ctl.pending_notifications());
    expect_recv_report(send_ctl.next_recv_notification(), recv1_time, Recv1Cname,
                       Recv1Ssrc, SendSsrc, Seed2);

    advance_time(send_time);
    advance_time(recv2_time);

    // Generate receiver 2 report
    recv2_ctl.set_recv_report(
        0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, SendSsrc, Seed3));
    CHECK_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));
    CHECK_EQUAL(1, recv2_comm.num_streams());
    CHECK_EQUAL(1, recv2_queue.size());

    // Deliver receiver 1 report to sender
    CHECK_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv2_queue), send_time));
    CHECK_EQUAL(2, send_comm.num_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_ctl.pending_notifications());
    expect_recv_report(send_ctl.next_recv_notification(), recv2_time, Recv2Cname,
                       Recv2Ssrc, SendSsrc, Seed3);
}

TEST(communicator, receiver_report_first) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver report
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver receiver report to sender
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.num_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_ctl.pending_notifications());
    expect_recv_report(send_ctl.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       SendSsrc, Seed1);

    advance_time(send_time);
    advance_time(recv_time);

    // Generate sender report
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(1, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed2);
}

TEST(communicator, bidirectional_peers) {
    enum { Peer1Ssrc = 11, Peer2Ssrc = 22, Seed1 = 100, Seed2 = 200, Seed3 = 300 };

    const char* Peer1Cname = "peer1_cname";
    const char* Peer2Cname = "peer2_cname";

    Config config;

    packet::Queue peer1_queue;
    MockController peer1_ctl(Peer1Cname, Peer1Ssrc);
    Communicator peer1_comm(config, peer1_ctl, &peer1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(peer1_comm.is_valid());

    packet::Queue peer2_queue;
    MockController peer2_ctl(Peer2Cname, Peer2Ssrc);
    Communicator peer2_comm(config, peer2_ctl, &peer2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(peer2_comm.is_valid());

    core::nanoseconds_t peer1_time = 10000000000000000;
    core::nanoseconds_t peer2_time = 30000000000000000;

    // Generate report from peer 1
    peer1_ctl.set_send_report(make_send_report(peer1_time, Peer1Cname, Peer1Ssrc, Seed1));
    peer1_ctl.set_recv_report(
        0, make_recv_report(peer1_time, Peer1Cname, Peer1Ssrc, Peer2Ssrc, Seed1));
    CHECK_EQUAL(status::StatusOK, peer1_comm.generate_reports(peer1_time));
    CHECK_EQUAL(1, peer1_comm.num_streams());
    CHECK_EQUAL(1, peer1_queue.size());

    // Deliver report to peer 2
    CHECK_EQUAL(status::StatusOK,
                peer2_comm.process_packet(read_packet(peer1_queue), peer2_time));
    CHECK_EQUAL(1, peer2_comm.num_streams());

    // Check notifications on peer 2
    CHECK_EQUAL(1, peer2_ctl.pending_notifications());
    expect_send_report(peer2_ctl.next_send_notification(), peer1_time, Peer1Cname,
                       Peer1Ssrc, Seed1);

    advance_time(peer1_time);
    advance_time(peer2_time);

    // Generate report from peer 2
    peer2_ctl.set_send_report(make_send_report(peer2_time, Peer2Cname, Peer2Ssrc, Seed2));
    peer2_ctl.set_recv_report(
        0, make_recv_report(peer2_time, Peer2Cname, Peer2Ssrc, Peer1Ssrc, Seed2));
    CHECK_EQUAL(status::StatusOK, peer2_comm.generate_reports(peer2_time));
    CHECK_EQUAL(1, peer2_comm.num_streams());
    CHECK_EQUAL(1, peer2_queue.size());

    // Deliver report to peer 1
    CHECK_EQUAL(status::StatusOK,
                peer1_comm.process_packet(read_packet(peer2_queue), peer1_time));
    CHECK_EQUAL(1, peer1_comm.num_streams());

    // Check notifications on peer 1
    CHECK_EQUAL(2, peer1_ctl.pending_notifications());
    expect_send_report(peer1_ctl.next_send_notification(), peer2_time, Peer2Cname,
                       Peer2Ssrc, Seed2);
    expect_recv_report(peer1_ctl.next_recv_notification(), peer2_time, Peer2Cname,
                       Peer2Ssrc, Peer1Ssrc, Seed2);

    advance_time(peer1_time);
    advance_time(peer2_time);

    // Generate report from peer 1
    peer1_ctl.set_send_report(make_send_report(peer1_time, Peer1Cname, Peer1Ssrc, Seed3));
    peer1_ctl.set_recv_report(
        0, make_recv_report(peer1_time, Peer1Cname, Peer1Ssrc, Peer2Ssrc, Seed3));
    CHECK_EQUAL(status::StatusOK, peer1_comm.generate_reports(peer1_time));
    CHECK_EQUAL(1, peer1_comm.num_streams());
    CHECK_EQUAL(1, peer1_queue.size());

    // Deliver report to peer 2
    CHECK_EQUAL(status::StatusOK,
                peer2_comm.process_packet(read_packet(peer1_queue), peer2_time));
    CHECK_EQUAL(1, peer2_comm.num_streams());

    // Check notifications on peer 2
    CHECK_EQUAL(2, peer2_ctl.pending_notifications());
    expect_send_report(peer2_ctl.next_send_notification(), peer1_time, Peer1Cname,
                       Peer1Ssrc, Seed3);
    expect_recv_report(peer2_ctl.next_recv_notification(), peer1_time, Peer1Cname,
                       Peer1Ssrc, Peer2Ssrc, Seed3);
}

TEST(communicator, long_run) {
    enum {
        Send1Ssrc = 11,
        Send2Ssrc = 22,
        Recv1Ssrc = 33,
        Recv2Ssrc = 44,
        NumIters = 20
    };

    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    Config config;

    packet::Queue send1_queue;
    MockController send1_ctl(Send1Cname, Send1Ssrc);
    Communicator send1_comm(config, send1_ctl, &send1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    packet::Queue send2_queue;
    MockController send2_ctl(Send2Cname, Send2Ssrc);
    Communicator send2_comm(config, send2_ctl, &send2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    packet::Queue recv1_queue;
    MockController recv1_ctl(Recv1Cname, Recv1Ssrc);
    Communicator recv1_comm(config, recv1_ctl, &recv1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(recv1_comm.is_valid());

    packet::Queue recv2_queue;
    MockController recv2_ctl(Recv2Cname, Recv2Ssrc);
    Communicator recv2_comm(config, recv2_ctl, &recv2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(recv2_comm.is_valid());

    core::nanoseconds_t send1_time = 10000000000000000;
    core::nanoseconds_t send2_time = 30000000000000000;
    core::nanoseconds_t recv1_time = 60000000000000000;
    core::nanoseconds_t recv2_time = 90000000000000000;

    unsigned seed = 100;

    for (size_t iter = 0; iter < NumIters; iter++) {
        packet::PacketPtr pp;

        // Generate sender 1 report
        const core::nanoseconds_t send1_report_time = send1_time;
        send1_ctl.set_send_report(
            make_send_report(send1_time, Send1Cname, Send1Ssrc, seed));
        CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));

        // Deliver sender 1 report to receiver 1 and 2
        pp = read_packet(send1_queue);
        CHECK_EQUAL(status::StatusOK, recv1_comm.process_packet(pp, recv1_time));
        CHECK_EQUAL(status::StatusOK, recv2_comm.process_packet(pp, recv2_time));

        advance_time(send1_time);
        advance_time(send2_time);
        advance_time(recv1_time);
        advance_time(recv2_time);

        // Generate sender 2 report
        const core::nanoseconds_t send2_report_time = send2_time;
        send2_ctl.set_send_report(
            make_send_report(send2_time, Send2Cname, Send2Ssrc, seed));
        CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));

        // Deliver sender 2 report to receiver 1 and 2
        pp = read_packet(send2_queue);
        CHECK_EQUAL(status::StatusOK, recv1_comm.process_packet(pp, recv1_time));
        CHECK_EQUAL(status::StatusOK, recv2_comm.process_packet(pp, recv2_time));

        advance_time(send1_time);
        advance_time(send2_time);
        advance_time(recv1_time);
        advance_time(recv2_time);

        // Generate receiver 1 report
        const core::nanoseconds_t recv1_report_time = recv1_time;
        recv1_ctl.set_recv_report(
            0, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, Send1Ssrc, seed));
        recv1_ctl.set_recv_report(
            1, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, Send2Ssrc, seed));
        CHECK_EQUAL(status::StatusOK, recv1_comm.generate_reports(recv1_time));

        // Deliver receiver 1 report to sender 1 and 2
        pp = read_packet(recv1_queue);
        CHECK_EQUAL(status::StatusOK, send1_comm.process_packet(pp, send1_time));
        CHECK_EQUAL(status::StatusOK, send2_comm.process_packet(pp, send2_time));

        advance_time(send1_time);
        advance_time(send2_time);
        advance_time(recv1_time);
        advance_time(recv2_time);

        // Generate receiver 2 report
        const core::nanoseconds_t recv2_report_time = recv2_time;
        recv2_ctl.set_recv_report(
            0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send1Ssrc, seed));
        recv2_ctl.set_recv_report(
            1, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send2Ssrc, seed));
        CHECK_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));

        // Deliver receiver 2 report to sender 1 and 2
        pp = read_packet(recv2_queue);
        CHECK_EQUAL(status::StatusOK, send1_comm.process_packet(pp, send1_time));
        CHECK_EQUAL(status::StatusOK, send2_comm.process_packet(pp, send2_time));

        advance_time(send1_time);
        advance_time(send2_time);
        advance_time(recv1_time);
        advance_time(recv2_time);

        // Check notifications on receiver 1
        CHECK_EQUAL(2, recv1_ctl.pending_notifications());
        expect_send_report(recv1_ctl.next_send_notification(), send1_report_time,
                           Send1Cname, Send1Ssrc, seed);
        expect_send_report(recv1_ctl.next_send_notification(), send2_report_time,
                           Send2Cname, Send2Ssrc, seed);

        // Check notifications on receiver 2
        CHECK_EQUAL(2, recv2_ctl.pending_notifications());
        expect_send_report(recv2_ctl.next_send_notification(), send1_report_time,
                           Send1Cname, Send1Ssrc, seed);
        expect_send_report(recv2_ctl.next_send_notification(), send2_report_time,
                           Send2Cname, Send2Ssrc, seed);

        // Check notifications on sender 1
        CHECK_EQUAL(2, send1_ctl.pending_notifications());
        expect_recv_report(send1_ctl.next_recv_notification(), recv1_report_time,
                           Recv1Cname, Recv1Ssrc, Send1Ssrc, seed);
        expect_recv_report(send1_ctl.next_recv_notification(), recv2_report_time,
                           Recv2Cname, Recv2Ssrc, Send1Ssrc, seed);

        // Check notifications on sender 2
        CHECK_EQUAL(2, send2_ctl.pending_notifications());
        expect_recv_report(send2_ctl.next_recv_notification(), recv1_report_time,
                           Recv1Cname, Recv1Ssrc, Send2Ssrc, seed);
        expect_recv_report(send2_ctl.next_recv_notification(), recv2_report_time,
                           Recv2Cname, Recv2Ssrc, Send2Ssrc, seed);

        seed++;
    }
}

// Check how stream is terminated when we receive BYE message
TEST(communicator, halt_goodbye) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1);

    advance_time(send_time);
    advance_time(recv_time);

    // Generate sender goodbye
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_goodbye(send_time));
    CHECK_EQUAL(0, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender goodbye to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(0, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    CHECK_EQUAL(SendSsrc, recv_ctl.next_halt_notification());
}

// Check how stream is terminated when we don't hear from it during timeout
TEST(communicator, halt_timeout) {
    enum { SendSsrc1 = 11, SendSsrc2 = 22, RecvSsrc = 33, Seed = 100, NumIters = 10 };

    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::Queue send1_queue;
    MockController send1_ctl(Send1Cname, SendSsrc1);
    Communicator send1_comm(config, send1_ctl, &send1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    packet::Queue send2_queue;
    MockController send2_ctl(Send2Cname, SendSsrc2);
    Communicator send2_comm(config, send2_ctl, &send2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send1_time = 10000000000000000;
    core::nanoseconds_t send2_time = 30000000000000000;
    core::nanoseconds_t recv_time = 60000000000000000;

    for (size_t i = 0; i < NumIters; i++) {
        // Remaining 2ms until timeout, will not trigger
        advance_time(send1_time, config.inactivity_timeout);
        advance_time(recv_time, config.inactivity_timeout - 2 * core::Millisecond);

        // Generate sender 1 report
        send1_ctl.set_send_report(
            make_send_report(send1_time, Send1Cname, SendSsrc1, Seed));
        CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
        CHECK_EQUAL(0, send1_comm.num_streams());
        CHECK_EQUAL(1, send1_queue.size());

        // Deliver sender 1 report to receiver
        CHECK_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send1_queue), recv_time));
        CHECK_EQUAL(i == 0 ? 1 : 2, recv_comm.num_streams());

        // Check notifications on receiver
        CHECK_EQUAL(1, recv_ctl.pending_notifications());
        expect_send_report(recv_ctl.next_send_notification(), send1_time, Send1Cname,
                           SendSsrc1, Seed);

        // Remaining 1ms until timeout, will not trigger
        advance_time(send2_time, config.inactivity_timeout);
        advance_time(recv_time, core::Millisecond);

        // Generate sender 2 report
        send2_ctl.set_send_report(
            make_send_report(send2_time, Send2Cname, SendSsrc2, Seed));
        CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
        CHECK_EQUAL(0, send2_comm.num_streams());
        CHECK_EQUAL(1, send2_queue.size());

        // Deliver sender 2 report to receiver
        CHECK_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send2_queue), recv_time));
        CHECK_EQUAL(2, recv_comm.num_streams());

        // Check notifications on receiver
        CHECK_EQUAL(1, recv_ctl.pending_notifications());
        expect_send_report(recv_ctl.next_send_notification(), send2_time, Send2Cname,
                           SendSsrc2, Seed);
    }

    // Timeout will trigger for sender 2
    advance_time(send1_time, config.inactivity_timeout);
    advance_time(recv_time, config.inactivity_timeout);

    // Generate sender 1 report
    send1_ctl.set_send_report(make_send_report(send1_time, Send1Cname, SendSsrc1, Seed));
    CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(0, send1_comm.num_streams());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender 1 report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(2, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send1_time, Send1Cname,
                       SendSsrc1, Seed);
    CHECK_EQUAL(SendSsrc2, recv_ctl.next_halt_notification());

    // Timeout will trigger for sender 1
    advance_time(recv_time, config.inactivity_timeout);

    // Generate receiver report for sender 1
    // We don't actually deliver it, just want to trigger timeout
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc1, Seed));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(0, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    CHECK_EQUAL(SendSsrc1, recv_ctl.next_halt_notification());
}

// Check how stream is terminated when we its CNAME changes
TEST(communicator, halt_cname_change) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCnameA = "send_cname_a";
    const char* SendCnameB = "send_cname_b";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::Queue send1_queue;
    MockController send1_ctl(SendCnameA, SendSsrc);
    Communicator send1_comm(config, send1_ctl, &send1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    packet::Queue send2_queue;
    MockController send2_ctl(SendCnameB, SendSsrc);
    Communicator send2_comm(config, send2_ctl, &send2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report with CNAME
    send1_ctl.set_send_report(make_send_report(send_time, SendCnameA, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send1_comm.num_streams());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, SendCnameA, SendSsrc,
                       Seed1);

    advance_time(send_time);
    advance_time(recv_time);

    // Generate sender report with same SSRC and different CNAME
    send2_ctl.set_send_report(make_send_report(send_time, SendCnameB, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send2_comm.num_streams());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(2, recv_ctl.pending_notifications());
    CHECK_EQUAL(SendSsrc, recv_ctl.next_halt_notification());
    expect_send_report(recv_ctl.next_send_notification(), send_time, SendCnameB, SendSsrc,
                       Seed2);
}

// When CNAME comes in one packet, and report in another, we should
// correctly merge everything together
TEST(communicator, cname_comes_earlier) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* NoCname = "<not used>";
    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send1_config;
    // First report will have SDES, but not SR/XR
    send1_config.enable_sr_rr = false;
    send1_config.enable_xr = false;
    send1_config.enable_sdes = true;
    packet::Queue send1_queue;
    MockController send1_ctl(SendCname, SendSsrc);
    Communicator send1_comm(send1_config, send1_ctl, &send1_queue, composer,
                            packet_factory, buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    Config send2_config;
    // Second report will have SR/XR, but not SDES
    send2_config.enable_sr_rr = true;
    send2_config.enable_xr = true;
    send2_config.enable_sdes = false;
    packet::Queue send2_queue;
    MockController send2_ctl(NoCname, SendSsrc);
    Communicator send2_comm(send2_config, send2_ctl, &send2_queue, composer,
                            packet_factory, buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    Config recv_config;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(recv_config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate first sender report
    send1_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send1_comm.num_streams());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver (no notifications)
    CHECK_EQUAL(0, recv_ctl.pending_notifications());

    advance_time(send_time);
    advance_time(recv_time);

    // Generate second sender report
    send2_ctl.set_send_report(make_send_report(send_time, NoCname, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send2_comm.num_streams());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver (got notification)
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed2);
}

// When report comes in one packet, and CNAME in another, we should
// correctly merge everything together
TEST(communicator, cname_comes_later) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* NoCname = "<not used>";
    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send1_config;
    // First report will have SR/XR, but not SDES
    send1_config.enable_sr_rr = true;
    send1_config.enable_xr = true;
    send1_config.enable_sdes = false;
    packet::Queue send1_queue;
    MockController send1_ctl(NoCname, SendSsrc);
    Communicator send1_comm(send1_config, send1_ctl, &send1_queue, composer,
                            packet_factory, buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    Config send2_config;
    // Second report will have SDES, but not SR/XR
    send2_config.enable_sr_rr = false;
    send2_config.enable_xr = false;
    send2_config.enable_sdes = true;
    packet::Queue send2_queue;
    MockController send2_ctl(SendCname, SendSsrc);
    Communicator send2_comm(send2_config, send2_ctl, &send2_queue, composer,
                            packet_factory, buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    Config recv_config;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(recv_config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate first sender report
    send1_ctl.set_send_report(make_send_report(send_time, NoCname, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send1_comm.num_streams());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver (no CNAME)
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, "", SendSsrc, Seed1);

    advance_time(recv_time);

    // Generate second sender report
    send2_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send2_comm.num_streams());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver (got CNAME)
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1);
}

// Collision detected in SR/XR from remote sender
// Remote sender has same SSRC as us
TEST(communicator, collision_send_report) {
    enum {
        Send1Ssrc = 11,        // sender 1
        Send2Ssrc = 22,        // sender 2
        RecvSsrcA = Send2Ssrc, // initial SSRC of receiver (collision w/ sender 2)
        RecvSsrcB = 33,        // updated SSRC of receiver
        Seed1 = 100,
        Seed2 = 200,
        Seed3 = 300,
        Seed4 = 400
    };

    const char* RecvCname = "recv_cname";
    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";

    Config config;

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrcA);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    packet::Queue send1_queue;
    MockController send1_ctl(Send1Cname, Send1Ssrc);
    Communicator send1_comm(config, send1_ctl, &send1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    packet::Queue send2_queue;
    MockController send2_ctl(Send2Cname, Send2Ssrc);
    Communicator send2_comm(config, send2_ctl, &send2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    core::nanoseconds_t recv_time = 10000000000000000;
    core::nanoseconds_t send1_time = 30000000000000000;
    core::nanoseconds_t send2_time = 60000000000000000;

    // Generate report from receiver to sender 1
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed1));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Deliver report from receiver to sender 1
    send1_ctl.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed1));
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    expect_recv_report(send1_ctl.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrcA, Send1Ssrc, Seed1);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate report from sender 2
    // Sender 2 has same SSRC as receiver
    send2_ctl.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(1, send2_queue.size());
    CHECK_EQUAL(0, send2_comm.num_streams());

    // Tell receiver controller which SSRC to use when requested
    // to update SSRC
    recv_ctl.set_changed_ssrc(RecvSsrcB);

    // Deliver report from sender 2 to receiver
    // Receiver should detect SSRC collision
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send2_time, Send2Cname,
                       Send2Ssrc, Seed2);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // Since receiver detected collision, it should generate BYE message with
    // old SSRC, and then request stream controller to change SSRC
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed3));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Check notifications on receiver
    // It should request stream controller to change SSRC
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    recv_ctl.next_ssrc_change_notification();

    // Deliver report from receiver to sender 1
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(0, send1_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    CHECK_EQUAL(RecvSsrcA, send1_ctl.next_halt_notification());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // It should use new SSRC now
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcB, Send1Ssrc, Seed4));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Deliver report from receiver to sender 1
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    expect_recv_report(send1_ctl.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrcB, Send1Ssrc, Seed4);
}

// Collision detected in RR/XR from remote receiver to us
// Remote receiver has same SSRC as us
TEST(communicator, collision_recv_report) {
    enum {
        Recv1Ssrc = 11,        // receiver 1
        Recv2Ssrc = 22,        // receiver 2
        SendSsrcA = Recv2Ssrc, // initial SSRC of sender (collision w/ receiver 2)
        SendSsrcB = 33,        // updated SSRC of sender
        Seed1 = 100,
        Seed2 = 200,
        Seed3 = 300,
        Seed4 = 400
    };

    const char* SendCname = "send_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    Config config;

    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrcA);
    Communicator send_comm(config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    packet::Queue recv1_queue;
    MockController recv1_ctl(Recv1Cname, Recv1Ssrc);
    Communicator recv1_comm(config, recv1_ctl, &recv1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(recv1_comm.is_valid());

    packet::Queue recv2_queue;
    MockController recv2_ctl(Recv2Cname, Recv2Ssrc);
    Communicator recv2_comm(config, recv2_ctl, &recv2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(recv2_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv1_time = 30000000000000000;
    core::nanoseconds_t recv2_time = 60000000000000000;

    // Generate report from sender
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrcA, Seed1));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver report from sender to receiver 1
    CHECK_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send_queue), recv1_time));
    CHECK_EQUAL(1, recv1_comm.num_streams());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_ctl.pending_notifications());
    expect_send_report(recv1_ctl.next_send_notification(), send_time, SendCname,
                       SendSsrcA, Seed1);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate report from receiver 2
    // Receiver 2 has same SSRC as sender
    recv2_ctl.set_recv_report(
        0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, SendSsrcA, Seed2));
    CHECK_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));
    CHECK_EQUAL(1, recv2_queue.size());
    CHECK_EQUAL(1, recv2_comm.num_streams());

    // Tell sender controller which SSRC to use when requested
    // to update SSRC
    send_ctl.set_changed_ssrc(SendSsrcB);

    // Deliver report from receiver 2 to sender
    // sender should ignore this report, because it's for another sender
    // However it should also detect SSRC collision because receiver 2 has
    // same SSRC as sender
    CHECK_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv2_queue), send_time));
    CHECK_EQUAL(1, send_comm.num_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_ctl.pending_notifications());
    expect_recv_report(send_ctl.next_recv_notification(), recv2_time, Recv2Cname,
                       Recv2Ssrc, SendSsrcA, Seed2);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate next report from sender to receiver 1
    // Since sender detected collision, it should generate BYE message with
    // old SSRC, and then request stream controller to change SSRC
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrcA, Seed3));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(1, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Check notifications on sender
    // It should request stream controller to change SSRC
    CHECK_EQUAL(1, send_ctl.pending_notifications());
    send_ctl.next_ssrc_change_notification();

    // Deliver report from sender to receiver 1
    CHECK_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send_queue), recv1_time));
    CHECK_EQUAL(0, recv1_comm.num_streams());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_ctl.pending_notifications());
    CHECK_EQUAL(SendSsrcA, recv1_ctl.next_halt_notification());

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate next report from sender to receiver 1
    // It should use new SSRC now
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrcB, Seed4));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(1, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver report from sender to receiver 1
    CHECK_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send_queue), recv1_time));
    CHECK_EQUAL(1, recv1_comm.num_streams());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_ctl.pending_notifications());
    expect_send_report(recv1_ctl.next_send_notification(), send_time, SendCname,
                       SendSsrcB, Seed4);
}

// Collision detected in unrelated RR/XR from remote receiver to remote sender
// Remote receiver has same SSRC as us
TEST(communicator, collision_unrelated_recv_report) {
    enum {
        Recv1Ssrc = 11,         // receiver 1
        Recv2Ssrc = 22,         // receiver 2
        Send1SsrcA = Recv2Ssrc, // initial SSRC of sender 1 (collision w/ receiver 2)
        Send1SsrcB = 33,        // updated SSRC of sender 1
        Send2Ssrc = 44,         // sender 2 (imaginary)
        Seed1 = 100,
        Seed2 = 200,
        Seed3 = 300,
        Seed4 = 400
    };

    const char* Send1Cname = "send1_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    Config config;

    packet::Queue send1_queue;
    MockController send1_ctl(Send1Cname, Send1SsrcA);
    Communicator send1_comm(config, send1_ctl, &send1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    packet::Queue recv1_queue;
    MockController recv1_ctl(Recv1Cname, Recv1Ssrc);
    Communicator recv1_comm(config, recv1_ctl, &recv1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(recv1_comm.is_valid());

    packet::Queue recv2_queue;
    MockController recv2_ctl(Recv2Cname, Recv2Ssrc);
    Communicator recv2_comm(config, recv2_ctl, &recv2_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(recv2_comm.is_valid());

    core::nanoseconds_t send1_time = 10000000000000000;
    core::nanoseconds_t recv1_time = 30000000000000000;
    core::nanoseconds_t recv2_time = 60000000000000000;

    // Generate report from sender 1
    send1_ctl.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1SsrcA, Seed1));
    CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(0, send1_comm.num_streams());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver report from sender 1 to receiver 1
    CHECK_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send1_queue), recv1_time));
    CHECK_EQUAL(1, recv1_comm.num_streams());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_ctl.pending_notifications());
    expect_send_report(recv1_ctl.next_send_notification(), send1_time, Send1Cname,
                       Send1SsrcA, Seed1);

    advance_time(send1_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate report from receiver 2 to imaginary sender 2
    // Receiver 2 has same SSRC as sender 1
    recv2_ctl.set_recv_report(
        0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send2Ssrc, Seed2));
    CHECK_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));
    CHECK_EQUAL(1, recv2_queue.size());
    CHECK_EQUAL(1, recv2_comm.num_streams());

    // Tell sender 1 controller which SSRC to use when requested
    // to update SSRC
    send1_ctl.set_changed_ssrc(Send1SsrcB);

    // Deliver report from receiver 2 to sender 1
    // Sender 1 should ignore this report, because it's for another sender
    // However it should also detect SSRC collision because receiver 2 has
    // same SSRC as sender 1
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv2_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());

    advance_time(send1_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate next report from sender 1 to receiver 1
    // Since sender 1 detected collision, it should generate BYE message with
    // old SSRC, and then request stream controller to change SSRC
    send1_ctl.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1SsrcA, Seed3));
    CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());
    CHECK_EQUAL(1, send1_queue.size());

    // Check notifications on sender 1
    // It should request stream controller to change SSRC
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    send1_ctl.next_ssrc_change_notification();

    // Deliver report from sender 1 to receiver 1
    CHECK_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send1_queue), recv1_time));
    CHECK_EQUAL(0, recv1_comm.num_streams());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_ctl.pending_notifications());
    CHECK_EQUAL(Send1SsrcA, recv1_ctl.next_halt_notification());

    advance_time(send1_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate next report from sender 1 to receiver 1
    // It should use new SSRC now
    send1_ctl.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1SsrcB, Seed4));
    CHECK_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver report from sender 1 to receiver 1
    CHECK_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send1_queue), recv1_time));
    CHECK_EQUAL(1, recv1_comm.num_streams());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_ctl.pending_notifications());
    expect_send_report(recv1_ctl.next_send_notification(), send1_time, Send1Cname,
                       Send1SsrcB, Seed4);
}

// Collision detected in SDES
// Remote peer has same SSRC and different CNAME
TEST(communicator, collision_sdes_different_cname) {
    enum {
        Send1Ssrc = 11,        // sender 1
        Send2Ssrc = 22,        // sender 2
        RecvSsrcA = Send2Ssrc, // initial SSRC of receiver (collision w/ sender 2)
        RecvSsrcB = 33,        // updated SSRC of receiver
        Seed1 = 100,
        Seed2 = 200,
        Seed3 = 300,
        Seed4 = 400
    };

    const char* RecvCname = "recv_cname";
    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";

    Config config;

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrcA);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    packet::Queue send1_queue;
    MockController send1_ctl(Send1Cname, Send1Ssrc);
    Communicator send1_comm(config, send1_ctl, &send1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    Config send2_config;
    // Sender 2 reports only SDES, without SR/XR
    send2_config.enable_sr_rr = false;
    send2_config.enable_xr = false;
    send2_config.enable_sdes = true;
    packet::Queue send2_queue;
    MockController send2_ctl(Send2Cname, Send2Ssrc);
    Communicator send2_comm(send2_config, send2_ctl, &send2_queue, composer,
                            packet_factory, buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    core::nanoseconds_t recv_time = 10000000000000000;
    core::nanoseconds_t send1_time = 30000000000000000;
    core::nanoseconds_t send2_time = 60000000000000000;

    // Generate report from receiver to sender 1
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed1));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Deliver report from receiver to sender 1
    send1_ctl.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed1));
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    expect_recv_report(send1_ctl.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrcA, Send1Ssrc, Seed1);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate report from sender 2
    // Sender 2 has same SSRC as receiver and different CNAME
    send2_ctl.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(1, send2_queue.size());
    CHECK_EQUAL(0, send2_comm.num_streams());

    // Tell receiver controller which SSRC to use when requested
    // to update SSRC
    recv_ctl.set_changed_ssrc(RecvSsrcB);

    // Deliver report from sender 2 to receiver
    // Receiver should detect SSRC collision
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(0, recv_ctl.pending_notifications());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // Since receiver detected collision, it should generate BYE message with
    // old SSRC, and then request stream controller to change SSRC
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed3));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Check notifications on receiver
    // It should request stream controller to change SSRC
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    recv_ctl.next_ssrc_change_notification();

    // Deliver report from receiver to sender 1
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(0, send1_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    CHECK_EQUAL(RecvSsrcA, send1_ctl.next_halt_notification());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // It should use new SSRC now
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcB, Send1Ssrc, Seed4));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Deliver report from receiver to sender 1
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    expect_recv_report(send1_ctl.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrcB, Send1Ssrc, Seed4);
}

// Collision detected in SDES
// Remote peer has same SSRC and same CNAME, which is considered
// to be network loop and not handled as collision
TEST(communicator, collision_sdes_same_cname) {
    enum {
        Send1Ssrc = 11,       // sender 1
        Send2Ssrc = 22,       // sender 2
        RecvSsrc = Send2Ssrc, // receiver (same as sender 2)
        Seed1 = 100,
        Seed2 = 200,
        Seed3 = 300
    };

    const char* Send1Cname = "test_cname1";
    const char* Send2Cname = "test_cname2";
    const char* RecvCname = Send2Cname; // (same as sender 2)

    Config config;

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    packet::Queue send1_queue;
    MockController send1_ctl(Send1Cname, Send1Ssrc);
    Communicator send1_comm(config, send1_ctl, &send1_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(send1_comm.is_valid());

    Config send2_config;
    // Sender 2 reports only SDES, without SR/XR
    send2_config.enable_sr_rr = false;
    send2_config.enable_xr = false;
    send2_config.enable_sdes = true;
    packet::Queue send2_queue;
    MockController send2_ctl(Send2Cname, Send2Ssrc);
    Communicator send2_comm(send2_config, send2_ctl, &send2_queue, composer,
                            packet_factory, buffer_factory, arena);
    CHECK(send2_comm.is_valid());

    core::nanoseconds_t recv_time = 10000000000000000;
    core::nanoseconds_t send1_time = 30000000000000000;
    core::nanoseconds_t send2_time = 60000000000000000;

    // Generate report from receiver to sender 1
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed1));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Deliver report from receiver to sender 1
    send1_ctl.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed1));
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    expect_recv_report(send1_ctl.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       Send1Ssrc, Seed1);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate report from sender 2
    // Sender 2 has same SSRC as receiver and same CNAME
    send2_ctl.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed2));
    CHECK_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(1, send2_queue.size());
    CHECK_EQUAL(0, send2_comm.num_streams());

    // Deliver report from sender 2 to receiver
    // Receiver should detect SSRC collision
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(0, recv_ctl.pending_notifications());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // No collision should be reported & handled
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed3));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(2, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(0, recv_ctl.pending_notifications());

    // Deliver report from receiver to sender 1
    CHECK_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.num_streams());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_ctl.pending_notifications());
    expect_recv_report(send1_ctl.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       Send1Ssrc, Seed3);
}

// Handle incoming packet from sender without SDES
// (Only SR and XR)
TEST(communicator, missing_sender_sdes) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    send_config.enable_sr_rr = true;
    send_config.enable_xr = true;
    send_config.enable_sdes = false;
    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(send_config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    Config recv_config;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(recv_config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    // Notification with empty CNAME should be generated
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, "", SendSsrc, Seed1);
}

// Handle incoming packet from receiver without SDES
// (Only RR and XR)
TEST(communicator, missing_receiver_sdes) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(send_config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    Config recv_config;
    recv_config.enable_sr_rr = true;
    recv_config.enable_xr = true;
    recv_config.enable_sdes = false;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(recv_config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver report
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Deliver receiver report to sender
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.num_streams());

    // Check notifications on sender
    // Notification with empty CNAME should be generated
    CHECK_EQUAL(1, send_ctl.pending_notifications());
    expect_recv_report(send_ctl.next_recv_notification(), recv_time, "", RecvSsrc,
                       SendSsrc, Seed2);
}

// Handle incoming packet from sender without SR
// (Only SDES and XR)
TEST(communicator, missing_sender_sr) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    send_config.enable_sr_rr = false;
    send_config.enable_xr = true;
    send_config.enable_sdes = true;
    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(send_config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    Config recv_config;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(recv_config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    // No notifications should be generated
    CHECK_EQUAL(0, recv_ctl.pending_notifications());
}

// Handle incoming packet from receiver without RR
// (Only SDES and XR)
TEST(communicator, missing_receiver_rr) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(send_config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    Config recv_config;
    recv_config.enable_sr_rr = false;
    recv_config.enable_xr = true;
    recv_config.enable_sdes = true;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(recv_config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver report
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Deliver receiver report to sender
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.num_streams());

    // Check notifications on sender
    // No notifications should be generated
    CHECK_EQUAL(0, send_ctl.pending_notifications());
}

// Handle incoming packet from sender without XR
// (Only SR and SDES)
TEST(communicator, missing_sender_xr) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    send_config.enable_sr_rr = true;
    send_config.enable_xr = false;
    send_config.enable_sdes = true;
    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(send_config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    Config recv_config;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(recv_config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.num_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    CHECK_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    // Notification with zero XR fields should be generated
    CHECK_EQUAL(1, recv_ctl.pending_notifications());
    expect_send_report(recv_ctl.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1, /* expect_xr = */ false);
}

// Handle incoming packet from receiver without XR
// (Only RR and SDES)
TEST(communicator, missing_receiver_xr) {
    enum { SendSsrc = 11, RecvSsrc = 22, Seed1 = 100, Seed2 = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(send_config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    Config recv_config;
    recv_config.enable_sr_rr = true;
    recv_config.enable_xr = false;
    recv_config.enable_sdes = true;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(recv_config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver report
    recv_ctl.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Deliver receiver report to sender
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    CHECK_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.num_streams());

    // Check notifications on sender
    // Notification with zero XR fields should be generated
    CHECK_EQUAL(1, send_ctl.pending_notifications());
    expect_recv_report(send_ctl.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       SendSsrc, Seed2, /* expect_xr = */ false);
}

// Sender report is too large and is split into multiple packets
TEST(communicator, split_sender_report) {
    enum { SendSsrc = 100, RecvSsrc = 200, NumReports = 30, NumPackets = 3, Seed = 100 };

    const char* SendCname = "send_cname";

    Config config;
    config.inactivity_timeout = core::Second * 999;

    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Prepare sender
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));

    // Generate reports from multiple receivers to sender to let sender discover them
    for (size_t idx = 0; idx < NumReports; idx++) {
        advance_time(send_time);
        advance_time(recv_time);

        packet::stream_source_t recv_ssrc = RecvSsrc + idx;
        char recv_cname[64] = {};
        snprintf(recv_cname, sizeof(recv_cname), "recv_cname%d", (int)recv_ssrc);

        packet::Queue recv_queue;
        MockController recv_ctl(recv_cname, recv_ssrc);
        Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                               buffer_factory, arena);
        CHECK(recv_comm.is_valid());

        // Generate receiver report
        recv_ctl.set_recv_report(
            0, make_recv_report(recv_time, recv_cname, recv_ssrc, SendSsrc, Seed));
        CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK_EQUAL(1, recv_comm.num_streams());
        CHECK_EQUAL(1, recv_queue.size());

        // Deliver receiver report to sender
        CHECK_EQUAL(status::StatusOK,
                    send_comm.process_packet(read_packet(recv_queue), send_time));

        // Check notifications on sender
        CHECK_EQUAL(1, send_ctl.pending_notifications());
        expect_recv_report(send_ctl.next_recv_notification(), recv_time, recv_cname,
                           recv_ssrc, SendSsrc, Seed);
    }

    advance_time(send_time);

    // Generate sender multi-packet report
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(NumReports, send_comm.num_streams());
    CHECK_EQUAL(NumPackets, send_queue.size());

    packet::stream_source_t recv_ssrc = RecvSsrc;
    char recv_cname[64] = {};
    snprintf(recv_cname, sizeof(recv_cname), "recv_cname%d", (int)recv_ssrc);

    packet::Queue recv_queue;
    MockController recv_ctl(recv_cname, recv_ssrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    // Deliver sender report packets to one of the receivers
    while (send_queue.size() != 0) {
        advance_time(recv_time);
        CHECK_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send_queue), recv_time));
    }
    CHECK_EQUAL(1, recv_comm.num_streams());

    // Check notifications on receiver
    CHECK_EQUAL(3, recv_ctl.pending_notifications());
    for (size_t p = 0; p < NumPackets; p++) {
        expect_send_report(recv_ctl.next_send_notification(), send_time, SendCname,
                           SendSsrc, Seed);
    }
}

// Receiver report is too large and is split into multiple packets
TEST(communicator, split_receiver_report) {
    enum { SendSsrc = 100, RecvSsrc = 200, NumReports = 15, NumPackets = 3, Seed = 100 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::Queue send_queue;
    MockController send_ctl(SendCname, SendSsrc);
    Communicator send_comm(config, send_ctl, &send_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(send_comm.is_valid());

    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver multi-packet report
    for (size_t idx = 0; idx < NumReports; idx++) {
        recv_ctl.set_recv_report(
            idx, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc + idx, Seed));
    }
    CHECK_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(NumReports, recv_comm.num_streams());
    CHECK_EQUAL(NumPackets, recv_queue.size());

    // Deliver receiver report packets to sender
    send_ctl.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    while (recv_queue.size() != 0) {
        advance_time(send_time);
        CHECK_EQUAL(status::StatusOK,
                    send_comm.process_packet(read_packet(recv_queue), send_time));
    }
    CHECK_EQUAL(1, send_comm.num_streams());

    // Check notifications on sender
    CHECK_EQUAL(NumPackets, send_ctl.pending_notifications());
    for (size_t p = 0; p < NumPackets; p++) {
        expect_recv_report(send_ctl.next_recv_notification(), recv_time, RecvCname,
                           RecvSsrc, SendSsrc, Seed);
    }
}

// Sender+receiver report is too large and is split into multiple packets
TEST(communicator, split_sender_receiver_report) {
    enum {
        LocalSsrc = 100,
        RemoteSsrc = 200,
        NumReports = 15,
        NumPackets = 6,
        Seed = 100
    };

    const char* LocalCname = "local_cname";

    Config config;
    config.inactivity_timeout = core::Second * 999;

    packet::Queue local_queue;
    MockController local_ctl(LocalCname, LocalSsrc);
    Communicator local_comm(config, local_ctl, &local_queue, composer, packet_factory,
                            buffer_factory, arena);
    CHECK(local_comm.is_valid());

    core::nanoseconds_t local_time = 10000000000000000;
    core::nanoseconds_t remote_time = 30000000000000000;

    // Prepare local peer
    local_ctl.set_send_report(make_send_report(local_time, LocalCname, LocalSsrc, Seed));

    // Generate reports from multiple remote peers to local peer
    for (size_t idx = 0; idx < NumReports; idx++) {
        advance_time(local_time);
        advance_time(remote_time);

        packet::stream_source_t remote_ssrc = RemoteSsrc + idx;
        char remote_cname[64] = {};
        snprintf(remote_cname, sizeof(remote_cname), "remote_cname%d", (int)remote_ssrc);

        packet::Queue remote_queue;
        MockController remote_ctl(remote_cname, remote_ssrc);
        Communicator remote_comm(config, remote_ctl, &remote_queue, composer,
                                 packet_factory, buffer_factory, arena);
        CHECK(remote_comm.is_valid());

        // Generate remote peer report
        remote_ctl.set_recv_report(
            0, make_recv_report(remote_time, remote_cname, remote_ssrc, LocalSsrc, Seed));
        CHECK_EQUAL(status::StatusOK, remote_comm.generate_reports(remote_time));
        CHECK_EQUAL(1, remote_comm.num_streams());
        CHECK_EQUAL(1, remote_queue.size());

        // Deliver remote peer report to local peer
        CHECK_EQUAL(status::StatusOK,
                    local_comm.process_packet(read_packet(remote_queue), local_time));

        // Check notifications on local peer
        CHECK_EQUAL(1, local_ctl.pending_notifications());
        expect_recv_report(local_ctl.next_recv_notification(), remote_time, remote_cname,
                           remote_ssrc, LocalSsrc, Seed);
    }

    advance_time(local_time);

    // Generate local peer multi-packet report
    local_ctl.set_send_report(make_send_report(local_time, LocalCname, LocalSsrc, Seed));
    for (size_t idx = 0; idx < NumReports; idx++) {
        local_ctl.set_recv_report(
            idx,
            make_recv_report(local_time, LocalCname, LocalSsrc, RemoteSsrc + idx, Seed));
    }
    CHECK_EQUAL(status::StatusOK, local_comm.generate_reports(local_time));
    CHECK_EQUAL(NumReports, local_comm.num_streams());
    CHECK_EQUAL(NumPackets, local_queue.size());

    packet::stream_source_t remote_ssrc = RemoteSsrc;
    char remote_cname[64] = {};
    snprintf(remote_cname, sizeof(remote_cname), "remote_cname%d", (int)remote_ssrc);

    packet::Queue remote_queue;
    MockController remote_ctl(remote_cname, remote_ssrc);
    Communicator remote_comm(config, remote_ctl, &remote_queue, composer, packet_factory,
                             buffer_factory, arena);
    CHECK(remote_comm.is_valid());

    // Deliver local peer report packets to one of the remote peers
    remote_ctl.set_send_report(
        make_send_report(remote_time, remote_cname, remote_ssrc, Seed));
    while (local_queue.size() != 0) {
        advance_time(remote_time);
        CHECK_EQUAL(status::StatusOK,
                    remote_comm.process_packet(read_packet(local_queue), remote_time));
    }
    CHECK_EQUAL(1, remote_comm.num_streams());

    // Check notifications on remote peer
    CHECK_EQUAL(NumPackets * 2, remote_ctl.pending_notifications());
    for (size_t p = 0; p < NumPackets; p++) {
        expect_send_report(remote_ctl.next_send_notification(), local_time, LocalCname,
                           LocalSsrc, Seed);
        expect_recv_report(remote_ctl.next_recv_notification(), local_time, LocalCname,
                           LocalSsrc, RemoteSsrc, Seed);
    }
}

TEST(communicator, generation_error) {
    enum { Ssrc = 11, Seed = 100 };

    const char* Cname = "test_cname";
    Config config;

    { // error from arena
        MockArena peer_arena;
        packet::Queue peer_queue;
        MockController peer_ctl(Cname, Ssrc);
        Communicator peer_comm(config, peer_ctl, &peer_queue, composer, packet_factory,
                               buffer_factory, peer_arena);
        CHECK(peer_comm.is_valid());

        core::nanoseconds_t peer_time = 10000000000000000;

        // Tell arena to fail
        peer_arena.set_fail(true);
        peer_ctl.set_send_report(make_send_report(peer_time, Cname, Ssrc, Seed));
        for (size_t idx = 0; idx < 50; idx++) {
            // Tell stream controller to report 50 streams to
            // force allocations from arena
            peer_ctl.set_recv_report(
                idx, make_recv_report(peer_time, Cname, Ssrc, Ssrc, Seed));
        }

        CHECK_EQUAL(status::StatusNoMem, peer_comm.generate_reports(peer_time));
        CHECK_EQUAL(1, peer_comm.num_streams());
        CHECK_EQUAL(0, peer_queue.size());
    }
    { // error from writer
        MockWriter peer_writer(status::StatusNoData);
        MockController peer_ctl(Cname, Ssrc);
        Communicator peer_comm(config, peer_ctl, &peer_writer, composer, packet_factory,
                               buffer_factory, arena);
        CHECK(peer_comm.is_valid());

        core::nanoseconds_t peer_time = 10000000000000000;

        peer_ctl.set_send_report(make_send_report(peer_time, Cname, Ssrc, Seed));
        CHECK_EQUAL(status::StatusNoData, peer_comm.generate_reports(peer_time));
        CHECK_EQUAL(0, peer_comm.num_streams());
        CHECK_EQUAL(1, peer_writer.call_count());
    }
    { // buffer factory w/ small buffers
        packet::Queue peer_queue;
        MockController peer_ctl(Cname, Ssrc);
        // factory creates 5-byte buffers
        core::BufferFactory<uint8_t> peer_factory(arena, 5);
        Communicator peer_comm(config, peer_ctl, &peer_queue, composer, packet_factory,
                               peer_factory, arena);
        CHECK(peer_comm.is_valid());

        core::nanoseconds_t peer_time = 10000000000000000;

        peer_ctl.set_send_report(make_send_report(peer_time, Cname, Ssrc, Seed));
        CHECK_EQUAL(status::StatusNoSpace, peer_comm.generate_reports(peer_time));
        CHECK_EQUAL(0, peer_comm.num_streams());
        CHECK_EQUAL(0, peer_queue.size());
    }
}

TEST(communicator, processing_error) {
    enum { RecvSsrc = 11, Seed = 100 };

    const char* RecvCname = "recv_cname";

    Config config;
    config.inactivity_timeout = core::Second * 999;

    MockArena recv_arena;
    packet::Queue recv_queue;
    MockController recv_ctl(RecvCname, RecvSsrc);
    Communicator recv_comm(config, recv_ctl, &recv_queue, composer, packet_factory,
                           buffer_factory, recv_arena);
    CHECK(recv_comm.is_valid());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Tell receiver's arena to fail
    recv_arena.set_fail(true);

    packet::stream_source_t send_ssrc = 100;
    size_t n_reports = 0;

    for (;;) {
        n_reports++;
        send_ssrc++;
        char send_cname[64] = {};
        snprintf(send_cname, sizeof(send_cname), "send_cname%d", (int)send_ssrc);

        advance_time(send_time);
        advance_time(recv_time);

        packet::Queue send_queue;
        MockController send_ctl(send_cname, send_ssrc);
        Communicator send_comm(config, send_ctl, &send_queue, composer, packet_factory,
                               buffer_factory, arena);
        CHECK(send_comm.is_valid());

        // Generate sender report
        send_ctl.set_send_report(
            make_send_report(send_time, send_cname, send_ssrc, Seed));
        CHECK_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK_EQUAL(0, send_comm.num_streams());
        CHECK_EQUAL(1, send_queue.size());

        // Deliver sender report to receiver
        const status::StatusCode status =
            recv_comm.process_packet(read_packet(send_queue), recv_time);

        if (status == status::StatusOK) {
            // Check notifications on receiver
            CHECK_EQUAL(n_reports, recv_comm.num_streams());
            CHECK_EQUAL(1, recv_ctl.pending_notifications());
            expect_send_report(recv_ctl.next_send_notification(), send_time, send_cname,
                               send_ssrc, Seed);

            // Repeat until failure.
            // First few iterations will succeed because arena is not used
            // until pre-allocated capacity is full.
            continue;
        } else {
            // Finally allocation failed and reported.
            CHECK_EQUAL(status::StatusNoMem, status);
            CHECK_EQUAL(n_reports - 1, recv_comm.num_streams());
            CHECK_EQUAL(0, recv_ctl.pending_notifications());
            break;
        }
    }
}

} // namespace rtcp
} // namespace roc
