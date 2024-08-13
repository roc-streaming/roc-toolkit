/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr_to_str.h"
#include "roc_core/heap_arena.h"
#include "roc_core/time.h"
#include "roc_packet/fifo_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtcp/communicator.h"
#include "roc_rtcp/composer.h"
#include "roc_rtcp/iparticipant.h"
#include "roc_rtcp/print_packet.h"

// This file contains tests that check how rtcp::Communicator allows IParticipant
// instances (senders and receivers) to exchange RTCP reports.
//
// Tests create separate IParticipant + Communicator for every sender or receiver.
// IParticipant is implemented by a mock. Tests instruct mock to return specific
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

enum { MaxPacketSz = 1000, SmallPacketSz = 500, TinyPacketSz = 5, SampleRate = 50000 };

enum {
    Seed = 100,
    Seed1 = 100,
    Seed2 = 200,
    Seed3 = 300,
    Seed4 = 400,
    Seed5 = 500,
    Seed6 = 600,
    Seed7 = 700,
    Seed8 = 800
};

// Precision of most timestamps
// The error is introduced by conversion between NTP and Unix time.
const core::nanoseconds_t TimestampEpsilon = 1 * core::Nanosecond;

// LSR/LRR and DLSR/DLRR precision in RTCP is ~16us because low 16 bits
// of 64-bit NTP timestamp are truncated.
const core::nanoseconds_t RttEpsilon = 16 * core::Microsecond;

core::HeapArena arena;

packet::PacketFactory packet_factory(arena, MaxPacketSz);
packet::PacketFactory small_packet_factory(arena, SmallPacketSz);
packet::PacketFactory tiny_packet_factory(arena, TinyPacketSz);

Composer composer(arena);

// Mock implementation of IParticipant
struct MockParticipant : public IParticipant, public core::NonCopyable<> {
    MockParticipant(const char* cname,
                    packet::stream_source_t source_id,
                    ParticipantReportMode report_mode) {
        status_ = status::StatusOK;
        cname_ = cname;
        source_id_ = changed_source_id_ = source_id;
        report_mode_ = report_mode;
        has_send_report_ = false;
        memset(has_recv_report_, 0, sizeof(has_recv_report_));
        cur_send_notification_ = num_send_notifications_ = 0;
        cur_recv_notification_ = num_recv_notifications_ = 0;
        cur_halt_notification_ = num_halt_notifications_ = 0;
        memset(halt_notifications_, 0, sizeof(halt_notifications_));
        num_ssrc_change_notifications_ = 0;
    }

    ~MockParticipant() {
        // Every test should fetch and check all pending notifications.
        CHECK_EQUAL(0, pending_notifications());
    }

    void set_status(status::StatusCode status) {
        status_ = status;
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

    void set_report_address(const address::SocketAddr& report_addr) {
        report_addr_ = report_addr;
    }

    size_t pending_notifications() const {
        return (num_send_notifications_ - cur_send_notification_)
            + (num_recv_notifications_ - cur_recv_notification_)
            + (num_halt_notifications_ - cur_halt_notification_)
            + num_ssrc_change_notifications_;
    }

    SendReport next_send_notification() {
        CHECK(cur_recv_notification_ < num_recv_notifications_);
        return recv_notifications_[cur_recv_notification_++ % MaxNotifications];
    }

    RecvReport next_recv_notification() {
        CHECK(cur_send_notification_ < num_send_notifications_);
        return send_notifications_[cur_send_notification_++ % MaxNotifications];
    }

    packet::stream_source_t next_halt_notification() {
        CHECK(cur_halt_notification_ < num_halt_notifications_);
        return halt_notifications_[cur_halt_notification_++ % MaxNotifications];
    }

    void next_ssrc_change_notification() {
        CHECK(num_ssrc_change_notifications_ > 0);
        num_ssrc_change_notifications_--;
    }

    virtual ParticipantInfo participant_info() {
        ParticipantInfo part_info;
        part_info.cname = cname_;
        part_info.source_id = source_id_;
        part_info.report_mode = report_mode_;
        part_info.report_address = report_addr_;
        return part_info;
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
        CHECK(core::ns_equal_delta(report_time, send_report_.report_timestamp,
                                   core::Nanosecond));

        SendReport report = send_report_;
        report.report_timestamp = report_time;

        return report;
    }

    virtual status::StatusCode notify_send_stream(packet::stream_source_t recv_source_id,
                                                  const RecvReport& recv_report) {
        CHECK_EQUAL(recv_source_id, recv_report.receiver_source_id);
        if (status_ == status::StatusOK) {
            CHECK(cur_send_notification_ <= num_send_notifications_);
            CHECK(num_send_notifications_ - cur_send_notification_ < MaxNotifications);
            send_notifications_[num_send_notifications_++ % MaxNotifications] =
                recv_report;
        }
        return status_;
    }

    virtual size_t num_recv_streams() {
        size_t cnt = 0;
        for (size_t i = 0; i < MaxStreams; i++) {
            if (has_recv_report_[i]) {
                cnt++;
            }
        }
        return cnt;
    }

    virtual void query_recv_streams(rtcp::RecvReport* reports,
                                    size_t n_reports,
                                    core::nanoseconds_t report_time) {
        CHECK(reports);
        CHECK_EQUAL(num_recv_streams(), n_reports);
        for (size_t n = 0; n < n_reports; n++) {
            CHECK(has_recv_report_[n]);
            CHECK(core::ns_equal_delta(report_time, recv_report_[n].report_timestamp,
                                       core::Nanosecond));

            reports[n] = recv_report_[n];
            reports[n].report_timestamp = report_time;
        }
    }

    virtual status::StatusCode notify_recv_stream(packet::stream_source_t send_source_id,
                                                  const SendReport& send_report) {
        CHECK_EQUAL(send_source_id, send_report.sender_source_id);
        if (status_ == status::StatusOK) {
            CHECK(cur_recv_notification_ <= num_recv_notifications_);
            CHECK(num_recv_notifications_ - cur_recv_notification_ < MaxNotifications);
            recv_notifications_[num_recv_notifications_++ % MaxNotifications] =
                send_report;
        }
        return status_;
    }

    virtual void halt_recv_stream(packet::stream_source_t send_source_id) {
        CHECK(cur_halt_notification_ <= num_halt_notifications_);
        CHECK(num_halt_notifications_ - cur_halt_notification_ < MaxNotifications);
        halt_notifications_[num_halt_notifications_++ % MaxNotifications] =
            send_source_id;
    }

private:
    enum { MaxStreams = 50, MaxNotifications = 50 };

    status::StatusCode status_;

    const char* cname_;

    packet::stream_source_t source_id_;
    packet::stream_source_t changed_source_id_;

    ParticipantReportMode report_mode_;
    address::SocketAddr report_addr_;

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

    virtual size_t compute_allocated_size(size_t size) const {
        return ha_.compute_allocated_size(size);
    }

    virtual size_t allocated_size(void* ptr) const {
        return ha_.allocated_size(ptr);
    }

    void set_fail(bool fail) {
        fail_ = fail;
    }

private:
    core::HeapArena ha_;
    bool fail_;
};

void expect_timestamp(const char* name,
                      core::nanoseconds_t expected,
                      core::nanoseconds_t actual,
                      core::nanoseconds_t epsilon) {
    if (!core::ns_equal_delta(expected, actual, epsilon)) {
        char sbuff[256];
        snprintf(sbuff, sizeof(sbuff),
                 "failed comparing %s timestamps:\n"
                 " expected:  %lld\n"
                 " actual:    %lld\n"
                 " delta:     %lld\n"
                 " max_delta: %lld\n",
                 name, (long long)expected, (long long)actual,
                 (long long)(expected - actual), (long long)epsilon);
        FAIL(sbuff);
    }
}

SendReport make_send_report(core::nanoseconds_t time,
                            const char* send_cname,
                            packet::stream_source_t send_ssrc,
                            unsigned int seed) {
    SendReport report;
    report.sender_source_id = send_ssrc;
    report.sender_cname = send_cname;
    report.report_timestamp = time;
    report.stream_timestamp = seed * 1000;
    report.sample_rate = SampleRate;
    report.packet_count = seed * 2000;
    report.byte_count = seed * 3000;
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
    expect_timestamp("report_timestamp", time, report.report_timestamp, TimestampEpsilon);
    CHECK_EQUAL(seed * 1000, report.stream_timestamp);
    CHECK_EQUAL(0, report.sample_rate);
    CHECK_EQUAL(seed * 2000, report.packet_count);
    CHECK_EQUAL(seed * 3000, report.byte_count);
    if (expect_xr) {
        CHECK(report.rtt >= 0);
    } else {
        CHECK(report.rtt == 0);
        CHECK(report.clock_offset == 0);
    }
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
    report.sample_rate = SampleRate;
    report.ext_first_seqnum = seed * 10;
    report.ext_last_seqnum = seed * 2000;
    report.cum_loss = (long)seed * 3000;
    report.jitter = seed * 400000;
    report.niq_latency = seed * 500000;
    report.niq_stalling = seed * 600000;
    report.e2e_latency = seed * 7000;
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
    if (expect_xr) {
        expect_timestamp("report_timestamp", time, report.report_timestamp,
                         TimestampEpsilon);
    } else {
        CHECK(report.report_timestamp == 0);
    }
    CHECK_EQUAL(0, report.sample_rate);
    if (expect_xr) {
        CHECK_EQUAL(seed * 10, report.ext_first_seqnum);
    } else {
        CHECK_EQUAL(0, report.ext_first_seqnum);
    }
    CHECK_EQUAL(seed * 2000, report.ext_last_seqnum);
    if (expect_xr) {
        CHECK_EQUAL(report.ext_last_seqnum - report.ext_first_seqnum + 1,
                    report.packet_count);
    }
    CHECK_EQUAL((long)seed * 3000, report.cum_loss);
    expect_timestamp("jitter", seed * 400000, report.jitter, TimestampEpsilon);
    if (expect_xr) {
        expect_timestamp("niq_latency", seed * 500000, report.niq_latency, RttEpsilon);
        expect_timestamp("niq_stalling", seed * 600000, report.niq_stalling, RttEpsilon);
        expect_timestamp("e2e_latency", seed * 7000, report.e2e_latency,
                         TimestampEpsilon);
        CHECK(report.rtt >= 0);
    } else {
        CHECK(report.niq_latency == 0);
        CHECK(report.niq_stalling == 0);
        CHECK(report.e2e_latency == 0);
        CHECK(report.rtt == 0);
        CHECK(report.clock_offset == 0);
    }
}

packet::PacketPtr read_packet(packet::FifoQueue& source) {
    CHECK(source.size() != 0);
    packet::PacketPtr pp;
    LONGS_EQUAL(status::StatusOK, source.read(pp, packet::ModeFetch));
    CHECK(pp);
    CHECK(pp->rtcp());
    CHECK(pp->rtcp()->payload);
    roc_log(LogTrace, "delivering rtcp packet");
    if (core::Logger::instance().get_level() >= LogTrace) {
        print_packet(pp->rtcp()->payload);
    }
    return pp;
}

address::SocketAddr make_address(int port) {
    address::SocketAddr addr;
    CHECK(addr.set_host_port(address::Family_IPv4, "127.0.0.1", port));
    return addr;
}

void set_src_address(const packet::PacketPtr& pp, const address::SocketAddr& address) {
    CHECK(pp);
    CHECK(pp->udp());

    pp->udp()->src_addr = address;
}

// Check that packet has specified destination address
void expect_has_dest_address(const packet::PacketPtr& pp,
                             const address::SocketAddr& address) {
    CHECK(pp);
    CHECK(pp->udp());

    if (pp->udp()->dst_addr != address) {
        char buf[1024] = {};
        snprintf(buf, sizeof(buf),
                 "packet dst_address doesn't match:\n"
                 "  expected: %s\n"
                 "  actual:   %s\n",
                 address::socket_addr_to_str(address).c_str(),
                 address::socket_addr_to_str(pp->udp()->dst_addr).c_str());
        FAIL(buf);
    }
}

// Check that report has blocks originated by given SSRC
void expect_has_orig_ssrc(const packet::PacketPtr& pp,
                          packet::stream_source_t ssrc,
                          bool present) {
    CHECK(pp);
    CHECK(pp->rtcp());
    CHECK(pp->rtcp()->payload);

    int ssrc_found = 0;

    Traverser traverser(pp->rtcp()->payload);
    CHECK(traverser.parse());

    Traverser::Iterator iter = traverser.iter();
    Traverser::Iterator::State state;

    while ((state = iter.next()) != Traverser::Iterator::END) {
        switch (state) {
        case Traverser::Iterator::SR: {
            if (iter.get_sr().ssrc() == ssrc) {
                ssrc_found++;
            }
        } break;

        case Traverser::Iterator::RR: {
            if (iter.get_rr().ssrc() == ssrc) {
                ssrc_found++;
            }
        } break;

        case Traverser::Iterator::XR: {
            XrTraverser xr = iter.get_xr();
            CHECK(xr.parse());

            if (xr.packet().ssrc() == ssrc) {
                ssrc_found++;
            }
        } break;

        default:
            break;
        }
    }

    if (present) {
        CHECK(ssrc_found > 0);
    } else {
        CHECK_EQUAL(0, ssrc_found);
    }
}

// Check that report has blocks targeted to given SSRC
void expect_has_dest_ssrc(const packet::PacketPtr& pp,
                          packet::stream_source_t ssrc,
                          bool present) {
    CHECK(pp);
    CHECK(pp->rtcp());
    CHECK(pp->rtcp()->payload);

    int ssrc_found = 0;

    Traverser traverser(pp->rtcp()->payload);
    CHECK(traverser.parse());

    Traverser::Iterator iter = traverser.iter();
    Traverser::Iterator::State state;

    while ((state = iter.next()) != Traverser::Iterator::END) {
        switch (state) {
        case Traverser::Iterator::SR: {
            for (size_t n = 0; n < iter.get_sr().num_blocks(); n++) {
                if (iter.get_sr().get_block(n).ssrc() == ssrc) {
                    ssrc_found++;
                }
            }
        } break;

        case Traverser::Iterator::RR: {
            for (size_t n = 0; n < iter.get_rr().num_blocks(); n++) {
                if (iter.get_rr().get_block(n).ssrc() == ssrc) {
                    ssrc_found++;
                }
            }
        } break;

        case Traverser::Iterator::XR: {
            XrTraverser xr = iter.get_xr();
            CHECK(xr.parse());

            XrTraverser::Iterator xr_iter = xr.iter();
            XrTraverser::Iterator::Iterator::State xr_state;

            while ((xr_state = xr_iter.next()) != XrTraverser::Iterator::END) {
                switch (xr_state) {
                case XrTraverser::Iterator::DLRR_BLOCK: {
                    for (size_t n = 0; n < xr_iter.get_dlrr().num_subblocks(); n++) {
                        if (xr_iter.get_dlrr().get_subblock(n).ssrc() == ssrc) {
                            ssrc_found++;
                        }
                    }
                } break;

                case XrTraverser::Iterator::MEASUREMENT_INFO_BLOCK: {
                    if (xr_iter.get_measurement_info().ssrc() == ssrc) {
                        ssrc_found++;
                    }
                } break;

                case XrTraverser::Iterator::DELAY_METRICS_BLOCK: {
                    if (xr_iter.get_delay_metrics().ssrc() == ssrc) {
                        ssrc_found++;
                    }
                } break;

                default:
                    break;
                }
            }
        } break;

        default:
            break;
        }
    }

    if (present) {
        CHECK(ssrc_found > 0);
    } else {
        CHECK_EQUAL(0, ssrc_found);
    }
}

void advance_time(core::nanoseconds_t& time, core::nanoseconds_t delta = core::Second) {
    time += delta;
}

} // namespace

TEST_GROUP(communicator) {};

TEST(communicator, one_sender_one_receiver) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1);

    advance_time(send_time);
    advance_time(recv_time);

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed2));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver receiver report to sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed3));
    LONGS_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       SendSsrc, Seed2);
}

TEST(communicator, two_senders_one_receiver) {
    enum { Send1Ssrc = 11, Send2Ssrc = 22, RecvSsrc = 33 };

    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, Send1Ssrc, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    packet::FifoQueue send2_queue;
    MockParticipant send2_part(Send2Cname, Send2Ssrc, Report_ToAddress);
    Communicator send2_comm(config, send2_part, send2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send1_time = 10000000000000000;
    core::nanoseconds_t send2_time = 30000000000000000;
    core::nanoseconds_t recv_time = 60000000000000000;

    // Generate sender 1 report
    send1_part.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed1));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(0, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender 1 report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send1_time, Send1Cname,
                       Send1Ssrc, Seed1);

    advance_time(recv_time);

    // Generate sender 2 report
    send2_part.set_send_report(
        make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed2));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(0, send2_comm.total_streams());
    CHECK_EQUAL(1, send2_comm.total_destinations());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender 2 report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send2_time, Send2Cname,
                       Send2Ssrc, Seed2);

    advance_time(send1_time);
    advance_time(send2_time);
    advance_time(recv_time);

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed3));
    recv_part.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed4));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver receiver report to sender 1 and 2
    packet::PacketPtr pp = read_packet(recv_queue);

    send1_part.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed5));
    LONGS_EQUAL(status::StatusOK, send1_comm.process_packet(pp, send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    send2_part.set_send_report(
        make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed6));
    LONGS_EQUAL(status::StatusOK, send2_comm.process_packet(pp, send2_time));
    CHECK_EQUAL(1, send2_comm.total_streams());
    CHECK_EQUAL(1, send2_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    expect_recv_report(send1_part.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrc, Send1Ssrc, Seed3);

    // Check notifications on sender 2
    CHECK_EQUAL(1, send2_part.pending_notifications());
    expect_recv_report(send2_part.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrc, Send2Ssrc, Seed4);
}

TEST(communicator, one_sender_two_receivers) {
    enum { SendSsrc = 11, Recv1Ssrc = 22, Recv2Ssrc = 33 };

    const char* SendCname = "send_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv1_queue;
    MockParticipant recv1_part(Recv1Cname, Recv1Ssrc, Report_Back);
    Communicator recv1_comm(config, recv1_part, recv1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv1_comm.init_status());

    packet::FifoQueue recv2_queue;
    MockParticipant recv2_part(Recv2Cname, Recv2Ssrc, Report_Back);
    Communicator recv2_comm(config, recv2_part, recv2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv2_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv1_time = 30000000000000000;
    core::nanoseconds_t recv2_time = 60000000000000000;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver 1 and 2
    packet::PacketPtr pp = read_packet(send_queue);
    LONGS_EQUAL(status::StatusOK, recv1_comm.process_packet(pp, recv1_time));
    CHECK_EQUAL(1, recv1_comm.total_streams());
    CHECK_EQUAL(0, recv1_comm.total_destinations());
    LONGS_EQUAL(status::StatusOK, recv2_comm.process_packet(pp, recv2_time));
    CHECK_EQUAL(1, recv2_comm.total_streams());
    CHECK_EQUAL(0, recv2_comm.total_destinations());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_part.pending_notifications());
    expect_send_report(recv1_part.next_send_notification(), send_time, SendCname,
                       SendSsrc, Seed1);

    // Check notifications on receiver 2
    CHECK_EQUAL(1, recv2_part.pending_notifications());
    expect_send_report(recv2_part.next_send_notification(), send_time, SendCname,
                       SendSsrc, Seed1);

    advance_time(send_time);
    advance_time(recv1_time);

    // Generate receiver 1 report
    recv1_part.set_recv_report(
        0, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, SendSsrc, Seed2));
    LONGS_EQUAL(status::StatusOK, recv1_comm.generate_reports(recv1_time));
    CHECK_EQUAL(1, recv1_comm.total_streams());
    CHECK_EQUAL(1, recv1_comm.total_destinations());
    CHECK_EQUAL(1, recv1_queue.size());

    // Deliver receiver 1 report to sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed3));
    LONGS_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv1_queue), send_time));
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv1_time, Recv1Cname,
                       Recv1Ssrc, SendSsrc, Seed2);

    advance_time(send_time);
    advance_time(recv2_time);

    // Generate receiver 2 report
    recv2_part.set_recv_report(
        0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, SendSsrc, Seed4));
    LONGS_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));
    CHECK_EQUAL(1, recv2_comm.total_streams());
    CHECK_EQUAL(1, recv2_comm.total_destinations());
    CHECK_EQUAL(1, recv2_queue.size());

    // Deliver receiver 1 report to sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed5));
    LONGS_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv2_queue), send_time));
    CHECK_EQUAL(2, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv2_time, Recv2Cname,
                       Recv2Ssrc, SendSsrc, Seed4);
}

TEST(communicator, receiver_report_first) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed1));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver receiver report to sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    LONGS_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       SendSsrc, Seed1);

    advance_time(send_time);
    advance_time(recv_time);

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed3));
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed2);
}

TEST(communicator, bidirectional_peers) {
    enum { Peer1Ssrc = 11, Peer2Ssrc = 22 };

    const char* Peer1Cname = "peer1_cname";
    const char* Peer2Cname = "peer2_cname";

    Config config;

    packet::FifoQueue peer1_queue;
    MockParticipant peer1_part(Peer1Cname, Peer1Ssrc, Report_ToAddress);
    Communicator peer1_comm(config, peer1_part, peer1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, peer1_comm.init_status());

    packet::FifoQueue peer2_queue;
    MockParticipant peer2_part(Peer2Cname, Peer2Ssrc, Report_ToAddress);
    Communicator peer2_comm(config, peer2_part, peer2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, peer2_comm.init_status());

    core::nanoseconds_t peer1_time = 10000000000000000;
    core::nanoseconds_t peer2_time = 30000000000000000;

    // Generate report from peer 1
    peer1_part.set_send_report(
        make_send_report(peer1_time, Peer1Cname, Peer1Ssrc, Seed1));
    peer1_part.set_recv_report(
        0, make_recv_report(peer1_time, Peer1Cname, Peer1Ssrc, Peer2Ssrc, Seed1));
    LONGS_EQUAL(status::StatusOK, peer1_comm.generate_reports(peer1_time));
    CHECK_EQUAL(1, peer1_comm.total_streams());
    CHECK_EQUAL(1, peer1_comm.total_destinations());
    CHECK_EQUAL(1, peer1_queue.size());

    // Deliver report to peer 2
    LONGS_EQUAL(status::StatusOK,
                peer2_comm.process_packet(read_packet(peer1_queue), peer2_time));
    CHECK_EQUAL(1, peer2_comm.total_streams());
    CHECK_EQUAL(0, peer2_comm.total_destinations());

    // Check notifications on peer 2
    CHECK_EQUAL(1, peer2_part.pending_notifications());
    expect_send_report(peer2_part.next_send_notification(), peer1_time, Peer1Cname,
                       Peer1Ssrc, Seed1);

    advance_time(peer1_time);
    advance_time(peer2_time);

    // Generate report from peer 2
    peer2_part.set_send_report(
        make_send_report(peer2_time, Peer2Cname, Peer2Ssrc, Seed3));
    peer2_part.set_recv_report(
        0, make_recv_report(peer2_time, Peer2Cname, Peer2Ssrc, Peer1Ssrc, Seed3));
    LONGS_EQUAL(status::StatusOK, peer2_comm.generate_reports(peer2_time));
    CHECK_EQUAL(1, peer2_comm.total_streams());
    CHECK_EQUAL(1, peer2_comm.total_destinations());
    CHECK_EQUAL(1, peer2_queue.size());

    // Deliver report to peer 1
    peer1_part.set_send_report(
        make_send_report(peer1_time, Peer1Cname, Peer1Ssrc, Seed4));
    peer1_part.set_recv_report(
        0, make_recv_report(peer1_time, Peer1Cname, Peer1Ssrc, Peer2Ssrc, Seed4));
    LONGS_EQUAL(status::StatusOK,
                peer1_comm.process_packet(read_packet(peer2_queue), peer1_time));
    CHECK_EQUAL(1, peer1_comm.total_streams());
    CHECK_EQUAL(1, peer1_comm.total_destinations());

    // Check notifications on peer 1
    CHECK_EQUAL(2, peer1_part.pending_notifications());
    expect_send_report(peer1_part.next_send_notification(), peer2_time, Peer2Cname,
                       Peer2Ssrc, Seed3);
    expect_recv_report(peer1_part.next_recv_notification(), peer2_time, Peer2Cname,
                       Peer2Ssrc, Peer1Ssrc, Seed3);

    advance_time(peer1_time);
    advance_time(peer2_time);

    // Generate report from peer 1
    peer1_part.set_send_report(
        make_send_report(peer1_time, Peer1Cname, Peer1Ssrc, Seed5));
    peer1_part.set_recv_report(
        0, make_recv_report(peer1_time, Peer1Cname, Peer1Ssrc, Peer2Ssrc, Seed5));
    LONGS_EQUAL(status::StatusOK, peer1_comm.generate_reports(peer1_time));
    CHECK_EQUAL(1, peer1_comm.total_streams());
    CHECK_EQUAL(1, peer1_comm.total_destinations());
    CHECK_EQUAL(1, peer1_queue.size());

    // Deliver report to peer 2
    peer2_part.set_send_report(
        make_send_report(peer2_time, Peer2Cname, Peer2Ssrc, Seed6));
    peer2_part.set_recv_report(
        0, make_recv_report(peer2_time, Peer2Cname, Peer2Ssrc, Peer1Ssrc, Seed6));
    LONGS_EQUAL(status::StatusOK,
                peer2_comm.process_packet(read_packet(peer1_queue), peer2_time));
    CHECK_EQUAL(1, peer2_comm.total_streams());
    CHECK_EQUAL(1, peer2_comm.total_destinations());

    // Check notifications on peer 2
    CHECK_EQUAL(2, peer2_part.pending_notifications());
    expect_send_report(peer2_part.next_send_notification(), peer1_time, Peer1Cname,
                       Peer1Ssrc, Seed5);
    expect_recv_report(peer2_part.next_recv_notification(), peer1_time, Peer1Cname,
                       Peer1Ssrc, Peer2Ssrc, Seed5);
}

TEST(communicator, long_run) {
    enum {
        Send1Ssrc = 11,
        Send2Ssrc = 22,
        Recv1Ssrc = 33,
        Recv2Ssrc = 44,
        NumIters = 50
    };

    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    Config config;

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, Send1Ssrc, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    packet::FifoQueue send2_queue;
    MockParticipant send2_part(Send2Cname, Send2Ssrc, Report_ToAddress);
    Communicator send2_comm(config, send2_part, send2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    packet::FifoQueue recv1_queue;
    MockParticipant recv1_part(Recv1Cname, Recv1Ssrc, Report_Back);
    Communicator recv1_comm(config, recv1_part, recv1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv1_comm.init_status());

    packet::FifoQueue recv2_queue;
    MockParticipant recv2_part(Recv2Cname, Recv2Ssrc, Report_Back);
    Communicator recv2_comm(config, recv2_part, recv2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv2_comm.init_status());

    core::nanoseconds_t send1_time = 10000000000000000;
    core::nanoseconds_t send2_time = 30000000000000000;
    core::nanoseconds_t recv1_time = 60000000000000000;
    core::nanoseconds_t recv2_time = 90000000000000000;

    unsigned seed = 100;

    for (size_t iter = 0; iter < NumIters; iter++) {
        packet::PacketPtr pp;

        // Generate sender 1 report
        const core::nanoseconds_t send1_report_time = send1_time;
        send1_part.set_send_report(
            make_send_report(send1_time, Send1Cname, Send1Ssrc, seed));
        LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));

        // Deliver sender 1 report to receiver 1 and 2
        pp = read_packet(send1_queue);

        if (iter != 0) {
            recv1_part.set_recv_report(
                0, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, Send1Ssrc, seed));
            recv1_part.set_recv_report(
                1, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, Send2Ssrc, seed));
        }
        LONGS_EQUAL(status::StatusOK, recv1_comm.process_packet(pp, recv1_time));

        if (iter != 0) {
            recv2_part.set_recv_report(
                0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send1Ssrc, seed));
            recv2_part.set_recv_report(
                1, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send2Ssrc, seed));
        }
        LONGS_EQUAL(status::StatusOK, recv2_comm.process_packet(pp, recv2_time));

        advance_time(send1_time);
        advance_time(send2_time);
        advance_time(recv1_time);
        advance_time(recv2_time);

        // Generate sender 2 report
        const core::nanoseconds_t send2_report_time = send2_time;
        send2_part.set_send_report(
            make_send_report(send2_time, Send2Cname, Send2Ssrc, seed));
        LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));

        // Deliver sender 2 report to receiver 1 and 2
        pp = read_packet(send2_queue);

        if (iter != 0) {
            recv1_part.set_recv_report(
                0, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, Send1Ssrc, seed));
            recv1_part.set_recv_report(
                1, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, Send2Ssrc, seed));
        }
        LONGS_EQUAL(status::StatusOK, recv1_comm.process_packet(pp, recv1_time));

        if (iter != 0) {
            recv2_part.set_recv_report(
                0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send1Ssrc, seed));
            recv2_part.set_recv_report(
                1, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send2Ssrc, seed));
        }
        LONGS_EQUAL(status::StatusOK, recv2_comm.process_packet(pp, recv2_time));

        advance_time(send1_time);
        advance_time(send2_time);
        advance_time(recv1_time);
        advance_time(recv2_time);

        // Generate receiver 1 report
        const core::nanoseconds_t recv1_report_time = recv1_time;
        recv1_part.set_recv_report(
            0, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, Send1Ssrc, seed));
        recv1_part.set_recv_report(
            1, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, Send2Ssrc, seed));
        LONGS_EQUAL(status::StatusOK, recv1_comm.generate_reports(recv1_time));

        // Deliver receiver 1 report to sender 1 and 2
        pp = read_packet(recv1_queue);

        send1_part.set_send_report(
            make_send_report(send1_time, Send1Cname, Send1Ssrc, seed));
        LONGS_EQUAL(status::StatusOK, send1_comm.process_packet(pp, send1_time));

        send2_part.set_send_report(
            make_send_report(send2_time, Send2Cname, Send2Ssrc, seed));
        LONGS_EQUAL(status::StatusOK, send2_comm.process_packet(pp, send2_time));

        advance_time(send1_time);
        advance_time(send2_time);
        advance_time(recv1_time);
        advance_time(recv2_time);

        // Generate receiver 2 report
        const core::nanoseconds_t recv2_report_time = recv2_time;
        recv2_part.set_recv_report(
            0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send1Ssrc, seed));
        recv2_part.set_recv_report(
            1, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send2Ssrc, seed));
        LONGS_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));

        // Deliver receiver 2 report to sender 1 and 2
        pp = read_packet(recv2_queue);

        send1_part.set_send_report(
            make_send_report(send1_time, Send1Cname, Send1Ssrc, seed));
        LONGS_EQUAL(status::StatusOK, send1_comm.process_packet(pp, send1_time));

        send2_part.set_send_report(
            make_send_report(send2_time, Send2Cname, Send2Ssrc, seed));
        LONGS_EQUAL(status::StatusOK, send2_comm.process_packet(pp, send2_time));

        advance_time(send1_time);
        advance_time(send2_time);
        advance_time(recv1_time);
        advance_time(recv2_time);

        // Check notifications on receiver 1
        CHECK_EQUAL(2, recv1_part.pending_notifications());
        expect_send_report(recv1_part.next_send_notification(), send1_report_time,
                           Send1Cname, Send1Ssrc, seed);
        expect_send_report(recv1_part.next_send_notification(), send2_report_time,
                           Send2Cname, Send2Ssrc, seed);

        // Check notifications on receiver 2
        CHECK_EQUAL(2, recv2_part.pending_notifications());
        expect_send_report(recv2_part.next_send_notification(), send1_report_time,
                           Send1Cname, Send1Ssrc, seed);
        expect_send_report(recv2_part.next_send_notification(), send2_report_time,
                           Send2Cname, Send2Ssrc, seed);

        // Check notifications on sender 1
        CHECK_EQUAL(2, send1_part.pending_notifications());
        expect_recv_report(send1_part.next_recv_notification(), recv1_report_time,
                           Recv1Cname, Recv1Ssrc, Send1Ssrc, seed);
        expect_recv_report(send1_part.next_recv_notification(), recv2_report_time,
                           Recv2Cname, Recv2Ssrc, Send1Ssrc, seed);

        // Check notifications on sender 2
        CHECK_EQUAL(2, send2_part.pending_notifications());
        expect_recv_report(send2_part.next_recv_notification(), recv1_report_time,
                           Recv1Cname, Recv1Ssrc, Send2Ssrc, seed);
        expect_recv_report(send2_part.next_recv_notification(), recv2_report_time,
                           Recv2Cname, Recv2Ssrc, Send2Ssrc, seed);

        seed++;
    }

    CHECK_EQUAL(0, send1_queue.size());
    CHECK_EQUAL(0, send2_queue.size());

    CHECK_EQUAL(0, recv1_queue.size());
    CHECK_EQUAL(0, recv2_queue.size());
}

// Check how stream is terminated when we receive BYE message
TEST(communicator, halt_goodbye) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1);

    advance_time(send_time);
    advance_time(recv_time);

    // Generate sender goodbye
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_goodbye(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender goodbye to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(0, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    CHECK_EQUAL(SendSsrc, recv_part.next_halt_notification());
}

// Check how stream is terminated when we don't hear from it during timeout
TEST(communicator, halt_timeout) {
    enum { SendSsrc1 = 11, SendSsrc2 = 22, RecvSsrc = 33, NumIters = 10 };

    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, SendSsrc1, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    packet::FifoQueue send2_queue;
    MockParticipant send2_part(Send2Cname, SendSsrc2, Report_ToAddress);
    Communicator send2_comm(config, send2_part, send2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send1_time = 10000000000000000;
    core::nanoseconds_t send2_time = 30000000000000000;
    core::nanoseconds_t recv_time = 60000000000000000;

    for (size_t iter = 0; iter < NumIters; iter++) {
        // Remaining 2ms until timeout, will not trigger
        advance_time(send1_time, config.inactivity_timeout);
        advance_time(recv_time, config.inactivity_timeout - 2 * core::Millisecond);

        // Generate sender 1 report
        send1_part.set_send_report(
            make_send_report(send1_time, Send1Cname, SendSsrc1, Seed));
        LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
        CHECK_EQUAL(0, send1_comm.total_streams());
        CHECK_EQUAL(1, send1_comm.total_destinations());
        CHECK_EQUAL(1, send1_queue.size());

        // Deliver sender 1 report to receiver
        LONGS_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send1_queue), recv_time));
        CHECK_EQUAL(iter == 0 ? 1 : 2, recv_comm.total_streams());
        CHECK_EQUAL(0, recv_comm.total_destinations());

        // Check notifications on receiver
        CHECK_EQUAL(1, recv_part.pending_notifications());
        expect_send_report(recv_part.next_send_notification(), send1_time, Send1Cname,
                           SendSsrc1, Seed);

        // Remaining 1ms until timeout, will not trigger
        advance_time(send2_time, config.inactivity_timeout);
        advance_time(recv_time, core::Millisecond);

        // Generate sender 2 report
        send2_part.set_send_report(
            make_send_report(send2_time, Send2Cname, SendSsrc2, Seed));
        LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
        CHECK_EQUAL(0, send2_comm.total_streams());
        CHECK_EQUAL(1, send2_comm.total_destinations());
        CHECK_EQUAL(1, send2_queue.size());

        // Deliver sender 2 report to receiver
        LONGS_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send2_queue), recv_time));
        CHECK_EQUAL(2, recv_comm.total_streams());
        CHECK_EQUAL(0, recv_comm.total_destinations());

        // Check notifications on receiver
        CHECK_EQUAL(1, recv_part.pending_notifications());
        expect_send_report(recv_part.next_send_notification(), send2_time, Send2Cname,
                           SendSsrc2, Seed);
    }

    // Timeout will trigger for sender 2
    advance_time(send1_time, config.inactivity_timeout);
    advance_time(recv_time, config.inactivity_timeout);

    // Generate sender 1 report
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, SendSsrc1, Seed));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(0, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender 1 report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(2, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send1_time, Send1Cname,
                       SendSsrc1, Seed);
    CHECK_EQUAL(SendSsrc2, recv_part.next_halt_notification());

    // Timeout will trigger for sender 1
    advance_time(recv_time, config.inactivity_timeout);

    // Generate receiver report for sender 1
    // We don't actually deliver it, just want to trigger timeout
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc1, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(0, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    CHECK_EQUAL(SendSsrc1, recv_part.next_halt_notification());
}

// Check how stream is terminated when we its CNAME changes
TEST(communicator, halt_cname_change) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCnameA = "send_cname_a";
    const char* SendCnameB = "send_cname_b";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(SendCnameA, SendSsrc, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    packet::FifoQueue send2_queue;
    MockParticipant send2_part(SendCnameB, SendSsrc, Report_ToAddress);
    Communicator send2_comm(config, send2_part, send2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report with CNAME
    send1_part.set_send_report(make_send_report(send_time, SendCnameA, SendSsrc, Seed1));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, SendCnameA,
                       SendSsrc, Seed1);

    advance_time(send_time);
    advance_time(recv_time);

    // Generate sender report with same SSRC and different CNAME
    send2_part.set_send_report(make_send_report(send_time, SendCnameB, SendSsrc, Seed2));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send2_comm.total_streams());
    CHECK_EQUAL(1, send2_comm.total_destinations());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(2, recv_part.pending_notifications());
    CHECK_EQUAL(SendSsrc, recv_part.next_halt_notification());
    expect_send_report(recv_part.next_send_notification(), send_time, SendCnameB,
                       SendSsrc, Seed2);
}

// When CNAME comes in one packet, and report in another, we should
// correctly merge everything together
TEST(communicator, cname_comes_earlier) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* NoCname = "<not used>";
    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send1_config;
    // First report will have SDES, but not SR/XR
    send1_config.enable_sr_rr = false;
    send1_config.enable_xr = false;
    send1_config.enable_sdes = true;
    packet::FifoQueue send1_queue;
    MockParticipant send1_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send1_comm(send1_config, send1_part, send1_queue, composer,
                            packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    Config send2_config;
    // Second report will have SR/XR, but not SDES
    send2_config.enable_sr_rr = true;
    send2_config.enable_xr = true;
    send2_config.enable_sdes = false;
    packet::FifoQueue send2_queue;
    MockParticipant send2_part(NoCname, SendSsrc, Report_ToAddress);
    Communicator send2_comm(send2_config, send2_part, send2_queue, composer,
                            packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    Config recv_config;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(recv_config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate first sender report
    send1_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed1));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver (no notifications)
    CHECK_EQUAL(0, recv_part.pending_notifications());

    advance_time(send_time);
    advance_time(recv_time);

    // Generate second sender report
    send2_part.set_send_report(make_send_report(send_time, NoCname, SendSsrc, Seed2));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send2_comm.total_streams());
    CHECK_EQUAL(1, send2_comm.total_destinations());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver (got notification)
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed2);
}

// When report comes in one packet, and CNAME in another, we should
// correctly merge everything together
TEST(communicator, cname_comes_later) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* NoCname = "<not used>";
    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send1_config;
    // First report will have SR/XR, but not SDES
    send1_config.enable_sr_rr = true;
    send1_config.enable_xr = true;
    send1_config.enable_sdes = false;
    packet::FifoQueue send1_queue;
    MockParticipant send1_part(NoCname, SendSsrc, Report_ToAddress);
    Communicator send1_comm(send1_config, send1_part, send1_queue, composer,
                            packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    Config send2_config;
    // Second report will have SDES, but not SR/XR
    send2_config.enable_sr_rr = false;
    send2_config.enable_xr = false;
    send2_config.enable_sdes = true;
    packet::FifoQueue send2_queue;
    MockParticipant send2_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send2_comm(send2_config, send2_part, send2_queue, composer,
                            packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    Config recv_config;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(recv_config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate first sender report
    send1_part.set_send_report(make_send_report(send_time, NoCname, SendSsrc, Seed1));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send1_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver (no CNAME)
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, "", SendSsrc,
                       Seed1);

    advance_time(recv_time);

    // Generate second sender report
    send2_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed2));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send2_comm.total_streams());
    CHECK_EQUAL(1, send2_comm.total_destinations());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());

    // Check notifications on receiver (got CNAME)
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed1);
}

// Collision detected in SR/XR from remote sender
// Remote sender has same SSRC as us
TEST(communicator, collision_send_report) {
    enum {
        Send1Ssrc = 11,        // sender 1
        Send2Ssrc = 22,        // sender 2
        RecvSsrcA = Send2Ssrc, // initial SSRC of receiver (collision w/ sender 2)
        RecvSsrcB = 33         // updated SSRC of receiver
    };

    const char* RecvCname = "recv_cname";
    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";

    Config config;

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrcA, Report_ToAddress);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, Send1Ssrc, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    packet::FifoQueue send2_queue;
    MockParticipant send2_part(Send2Cname, Send2Ssrc, Report_ToAddress);
    Communicator send2_comm(config, send2_part, send2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    core::nanoseconds_t recv_time = 10000000000000000;
    core::nanoseconds_t send1_time = 30000000000000000;
    core::nanoseconds_t send2_time = 60000000000000000;

    // Generate report from receiver to sender 1
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());

    // Deliver report from receiver to sender 1
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    expect_recv_report(send1_part.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrcA, Send1Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate report from sender 2
    // Sender 2 has same SSRC as receiver
    send2_part.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(1, send2_queue.size());
    CHECK_EQUAL(0, send2_comm.total_streams());
    CHECK_EQUAL(1, send2_comm.total_destinations());

    // Tell receiver participant which SSRC to use when requested
    // to update SSRC
    recv_part.set_changed_ssrc(RecvSsrcB);

    // Deliver report from sender 2 to receiver
    // Receiver should detect SSRC collision
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send2_time, Send2Cname,
                       Send2Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // Since receiver detected collision, it should generate BYE message with
    // old SSRC, and then request participant to change SSRC
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());

    // Check notifications on receiver
    // It should request participant to change SSRC
    CHECK_EQUAL(1, recv_part.pending_notifications());
    recv_part.next_ssrc_change_notification();

    // Deliver report from receiver to sender 1
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(0, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    CHECK_EQUAL(RecvSsrcA, send1_part.next_halt_notification());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // It should use new SSRC now
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcB, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_queue.size());
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());

    // Deliver report from receiver to sender 1
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    expect_recv_report(send1_part.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrcB, Send1Ssrc, Seed);
}

// Collision detected in RR/XR from remote receiver to us
// Remote receiver has same SSRC as us
TEST(communicator, collision_recv_report) {
    enum {
        Recv1Ssrc = 11,        // receiver 1
        Recv2Ssrc = 22,        // receiver 2
        SendSsrcA = Recv2Ssrc, // initial SSRC of sender (collision w/ receiver 2)
        SendSsrcB = 33         // updated SSRC of sender
    };

    const char* SendCname = "send_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrcA, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv1_queue;
    MockParticipant recv1_part(Recv1Cname, Recv1Ssrc, Report_ToAddress);
    Communicator recv1_comm(config, recv1_part, recv1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv1_comm.init_status());

    packet::FifoQueue recv2_queue;
    MockParticipant recv2_part(Recv2Cname, Recv2Ssrc, Report_ToAddress);
    Communicator recv2_comm(config, recv2_part, recv2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv2_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv1_time = 30000000000000000;
    core::nanoseconds_t recv2_time = 60000000000000000;

    // Generate report from sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrcA, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver report from sender to receiver 1
    LONGS_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send_queue), recv1_time));
    CHECK_EQUAL(1, recv1_comm.total_streams());
    CHECK_EQUAL(0, recv1_comm.total_destinations());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_part.pending_notifications());
    expect_send_report(recv1_part.next_send_notification(), send_time, SendCname,
                       SendSsrcA, Seed);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate report from receiver 2
    // Receiver 2 has same SSRC as sender
    recv2_part.set_recv_report(
        0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, SendSsrcA, Seed));
    LONGS_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));
    CHECK_EQUAL(1, recv2_comm.total_streams());
    CHECK_EQUAL(1, recv2_comm.total_destinations());
    CHECK_EQUAL(1, recv2_queue.size());

    // Tell sender participant which SSRC to use when requested
    // to update SSRC
    send_part.set_changed_ssrc(SendSsrcB);

    // Deliver report from receiver 2 to sender
    // sender should ignore this report, because it's for another sender
    // However it should also detect SSRC collision because receiver 2 has
    // same SSRC as sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrcA, Seed));
    LONGS_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv2_queue), send_time));
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv2_time, Recv2Cname,
                       Recv2Ssrc, SendSsrcA, Seed);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate next report from sender to receiver 1
    // Since sender detected collision, it should generate BYE message with
    // old SSRC, and then request participant to change SSRC
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrcA, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    // Check notifications on sender
    // It should request participant to change SSRC
    CHECK_EQUAL(1, send_part.pending_notifications());
    send_part.next_ssrc_change_notification();

    // Deliver report from sender to receiver 1
    LONGS_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send_queue), recv1_time));
    CHECK_EQUAL(0, recv1_comm.total_streams());
    CHECK_EQUAL(0, recv1_comm.total_destinations());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_part.pending_notifications());
    CHECK_EQUAL(SendSsrcA, recv1_part.next_halt_notification());

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate next report from sender to receiver 1
    // It should use new SSRC now
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrcB, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver report from sender to receiver 1
    LONGS_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send_queue), recv1_time));
    CHECK_EQUAL(1, recv1_comm.total_streams());
    CHECK_EQUAL(0, recv1_comm.total_destinations());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_part.pending_notifications());
    expect_send_report(recv1_part.next_send_notification(), send_time, SendCname,
                       SendSsrcB, Seed);
}

// Collision detected in unrelated RR/XR from remote receiver to remote sender
// Remote receiver has same SSRC as us
TEST(communicator, collision_unrelated_recv_report) {
    enum {
        Recv1Ssrc = 11,         // receiver 1
        Recv2Ssrc = 22,         // receiver 2
        Send1SsrcA = Recv2Ssrc, // initial SSRC of sender 1 (collision w/ receiver 2)
        Send1SsrcB = 33,        // updated SSRC of sender 1
        Send2Ssrc = 44          // sender 2 (imaginary)
    };

    const char* Send1Cname = "send1_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    Config config;

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, Send1SsrcA, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    packet::FifoQueue recv1_queue;
    MockParticipant recv1_part(Recv1Cname, Recv1Ssrc, Report_ToAddress);
    Communicator recv1_comm(config, recv1_part, recv1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv1_comm.init_status());

    packet::FifoQueue recv2_queue;
    MockParticipant recv2_part(Recv2Cname, Recv2Ssrc, Report_ToAddress);
    Communicator recv2_comm(config, recv2_part, recv2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv2_comm.init_status());

    core::nanoseconds_t send1_time = 10000000000000000;
    core::nanoseconds_t recv1_time = 30000000000000000;
    core::nanoseconds_t recv2_time = 60000000000000000;

    // Generate report from sender 1
    send1_part.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1SsrcA, Seed));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(0, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver report from sender 1 to receiver 1
    LONGS_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send1_queue), recv1_time));
    CHECK_EQUAL(1, recv1_comm.total_streams());
    CHECK_EQUAL(0, recv1_comm.total_destinations());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_part.pending_notifications());
    expect_send_report(recv1_part.next_send_notification(), send1_time, Send1Cname,
                       Send1SsrcA, Seed);

    advance_time(send1_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate report from receiver 2 to imaginary sender 2
    // Receiver 2 has same SSRC as sender 1
    recv2_part.set_recv_report(
        0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));
    CHECK_EQUAL(1, recv2_comm.total_streams());
    CHECK_EQUAL(1, recv2_comm.total_destinations());
    CHECK_EQUAL(1, recv2_queue.size());

    // Tell sender 1 participant which SSRC to use when requested
    // to update SSRC
    send1_part.set_changed_ssrc(Send1SsrcB);

    // Deliver report from receiver 2 to sender 1
    // Sender 1 should ignore this report, because it's for another sender
    // However it should also detect SSRC collision because receiver 2 has
    // same SSRC as sender 1
    send1_part.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1SsrcA, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv2_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    advance_time(send1_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate next report from sender 1 to receiver 1
    // Since sender 1 detected collision, it should generate BYE message with
    // old SSRC, and then request participant to change SSRC
    send1_part.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1SsrcA, Seed));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());
    CHECK_EQUAL(1, send1_queue.size());

    // Check notifications on sender 1
    // It should request participant to change SSRC
    CHECK_EQUAL(1, send1_part.pending_notifications());
    send1_part.next_ssrc_change_notification();

    // Deliver report from sender 1 to receiver 1
    send1_part.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1SsrcB, Seed));
    LONGS_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send1_queue), recv1_time));
    CHECK_EQUAL(0, recv1_comm.total_streams());
    CHECK_EQUAL(0, recv1_comm.total_destinations());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_part.pending_notifications());
    CHECK_EQUAL(Send1SsrcA, recv1_part.next_halt_notification());

    advance_time(send1_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate next report from sender 1 to receiver 1
    // It should use new SSRC now
    send1_part.set_send_report(
        make_send_report(send1_time, Send1Cname, Send1SsrcB, Seed));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver report from sender 1 to receiver 1
    LONGS_EQUAL(status::StatusOK,
                recv1_comm.process_packet(read_packet(send1_queue), recv1_time));
    CHECK_EQUAL(1, recv1_comm.total_streams());
    CHECK_EQUAL(0, recv1_comm.total_destinations());

    // Check notifications on receiver 1
    CHECK_EQUAL(1, recv1_part.pending_notifications());
    expect_send_report(recv1_part.next_send_notification(), send1_time, Send1Cname,
                       Send1SsrcB, Seed);
}

// Collision detected in SDES
// Remote peer has same SSRC and different CNAME
TEST(communicator, collision_sdes_different_cname) {
    enum {
        Send1Ssrc = 11,        // sender 1
        Send2Ssrc = 22,        // sender 2
        RecvSsrcA = Send2Ssrc, // initial SSRC of receiver (collision w/ sender 2)
        RecvSsrcB = 33         // updated SSRC of receiver
    };

    const char* RecvCname = "recv_cname";
    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";

    Config config;

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrcA, Report_ToAddress);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, Send1Ssrc, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    Config send2_config;
    // Sender 2 reports only SDES, without SR/XR
    send2_config.enable_sr_rr = false;
    send2_config.enable_xr = false;
    send2_config.enable_sdes = true;
    packet::FifoQueue send2_queue;
    MockParticipant send2_part(Send2Cname, Send2Ssrc, Report_ToAddress);
    Communicator send2_comm(send2_config, send2_part, send2_queue, composer,
                            packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    core::nanoseconds_t recv_time = 10000000000000000;
    core::nanoseconds_t send1_time = 30000000000000000;
    core::nanoseconds_t send2_time = 60000000000000000;

    // Generate report from receiver to sender 1
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver report from receiver to sender 1
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    expect_recv_report(send1_part.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrcA, Send1Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate report from sender 2
    // Sender 2 has same SSRC as receiver and different CNAME
    send2_part.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(0, send2_comm.total_streams());
    CHECK_EQUAL(1, send2_comm.total_destinations());
    CHECK_EQUAL(1, send2_queue.size());

    // Tell receiver participant which SSRC to use when requested
    // to update SSRC
    recv_part.set_changed_ssrc(RecvSsrcB);

    // Deliver report from sender 2 to receiver
    // Receiver should detect SSRC collision
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(0, recv_part.pending_notifications());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // Since receiver detected collision, it should generate BYE message with
    // old SSRC, and then request participant to change SSRC
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcA, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Check notifications on receiver
    // It should request participant to change SSRC
    CHECK_EQUAL(1, recv_part.pending_notifications());
    recv_part.next_ssrc_change_notification();

    // Deliver report from receiver to sender 1
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(0, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    CHECK_EQUAL(RecvSsrcA, send1_part.next_halt_notification());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // It should use new SSRC now
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrcB, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver report from receiver to sender 1
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    expect_recv_report(send1_part.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrcB, Send1Ssrc, Seed);
}

// Collision detected in SDES
// Remote peer has same SSRC and same CNAME, which is considered
// to be network loop and not handled as collision
TEST(communicator, collision_sdes_same_cname) {
    enum {
        Send1Ssrc = 11,      // sender 1
        Send2Ssrc = 22,      // sender 2
        RecvSsrc = Send2Ssrc // receiver (same as sender 2)
    };

    const char* Send1Cname = "test_cname1";
    const char* Send2Cname = "test_cname2";
    const char* RecvCname = Send2Cname; // (same as sender 2)

    Config config;

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, Send1Ssrc, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    Config send2_config;
    // Sender 2 reports only SDES, without SR/XR
    send2_config.enable_sr_rr = false;
    send2_config.enable_xr = false;
    send2_config.enable_sdes = true;
    packet::FifoQueue send2_queue;
    MockParticipant send2_part(Send2Cname, Send2Ssrc, Report_ToAddress);
    Communicator send2_comm(send2_config, send2_part, send2_queue, composer,
                            packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    core::nanoseconds_t recv_time = 10000000000000000;
    core::nanoseconds_t send1_time = 30000000000000000;
    core::nanoseconds_t send2_time = 60000000000000000;

    // Generate report from receiver to sender 1
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver report from receiver to sender 1
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    expect_recv_report(send1_part.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrc, Send1Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate report from sender 2
    // Sender 2 has same SSRC as receiver and same CNAME
    send2_part.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(0, send2_comm.total_streams());
    CHECK_EQUAL(1, send2_comm.total_destinations());
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver report from sender 2 to receiver
    // Receiver should detect SSRC collision
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send2_queue), recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());

    // Check notifications on receiver
    CHECK_EQUAL(0, recv_part.pending_notifications());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate next report from receiver to sender 1
    // No collision should be reported & handled
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    // Check notifications on receiver
    CHECK_EQUAL(0, recv_part.pending_notifications());

    // Deliver report from receiver to sender 1
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send1_comm.process_packet(read_packet(recv_queue), send1_time));
    CHECK_EQUAL(1, send1_comm.total_streams());
    CHECK_EQUAL(1, send1_comm.total_destinations());

    // Check notifications on sender 1
    CHECK_EQUAL(1, send1_part.pending_notifications());
    expect_recv_report(send1_part.next_recv_notification(), recv_time, RecvCname,
                       RecvSsrc, Send1Ssrc, Seed);
}

// Check how communicator handles reports looped back from itself
TEST(communicator, network_loop) {
    enum { LocalSsrc = 11, RemoteSsrc = 22 };

    const char* LocalCname = "local_cname";
    const char* RemoteCname = "remote_cname";

    Config config;

    packet::FifoQueue local_queue;
    MockParticipant local_part(LocalCname, LocalSsrc, Report_ToAddress);
    Communicator local_comm(config, local_part, local_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, local_comm.init_status());

    packet::FifoQueue remote_queue;
    MockParticipant remote_part(RemoteCname, RemoteSsrc, Report_ToAddress);
    Communicator remote_comm(config, remote_part, remote_queue, composer, packet_factory,
                             arena);
    LONGS_EQUAL(status::StatusOK, remote_comm.init_status());

    core::nanoseconds_t local_time = 10000000000000000;
    core::nanoseconds_t remote_time = 30000000000000000;

    // Generate report from local peer
    local_part.set_send_report(make_send_report(local_time, LocalCname, LocalSsrc, Seed));
    local_part.set_recv_report(
        0, make_recv_report(local_time, LocalCname, LocalSsrc, RemoteSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, local_comm.generate_reports(local_time));
    CHECK_EQUAL(1, local_queue.size());

    // Deliver report to remote peer
    LONGS_EQUAL(status::StatusOK,
                remote_comm.process_packet(read_packet(local_queue), remote_time));

    // Check notifications on remote peer
    CHECK_EQUAL(1, remote_part.pending_notifications());
    expect_send_report(remote_part.next_send_notification(), local_time, LocalCname,
                       LocalSsrc, Seed);

    advance_time(local_time);
    advance_time(remote_time);

    // Generate report from remote peer
    remote_part.set_send_report(
        make_send_report(remote_time, RemoteCname, RemoteSsrc, Seed));
    remote_part.set_recv_report(
        0, make_recv_report(remote_time, RemoteCname, RemoteSsrc, LocalSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, remote_comm.generate_reports(remote_time));
    CHECK_EQUAL(1, remote_queue.size());

    // Deliver report to local peer
    local_part.set_send_report(make_send_report(local_time, LocalCname, LocalSsrc, Seed));
    local_part.set_recv_report(
        0, make_recv_report(local_time, LocalCname, LocalSsrc, RemoteSsrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                local_comm.process_packet(read_packet(remote_queue), local_time));

    // Check notifications on local peer
    CHECK_EQUAL(2, local_part.pending_notifications());
    expect_send_report(local_part.next_send_notification(), remote_time, RemoteCname,
                       RemoteSsrc, Seed);
    expect_recv_report(local_part.next_recv_notification(), remote_time, RemoteCname,
                       RemoteSsrc, LocalSsrc, Seed);

    advance_time(local_time);
    advance_time(remote_time);

    // Generate report from local peer
    local_part.set_send_report(make_send_report(local_time, LocalCname, LocalSsrc, Seed));
    local_part.set_recv_report(
        0, make_recv_report(local_time, LocalCname, LocalSsrc, RemoteSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, local_comm.generate_reports(local_time));
    CHECK_EQUAL(1, local_queue.size());

    // Loop report back to local peer
    LONGS_EQUAL(status::StatusOK,
                local_comm.process_packet(read_packet(local_queue), local_time));

    // Expect no notifications
    CHECK_EQUAL(0, local_part.pending_notifications());

    advance_time(local_time);
    advance_time(remote_time);

    // Generate report from local peer
    local_part.set_send_report(make_send_report(local_time, LocalCname, LocalSsrc, Seed));
    local_part.set_recv_report(
        0, make_recv_report(local_time, LocalCname, LocalSsrc, RemoteSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, local_comm.generate_reports(local_time));
    CHECK_EQUAL(1, local_queue.size());

    // Inspect report
    packet::PacketPtr pp = read_packet(local_queue);

    expect_has_orig_ssrc(pp, LocalSsrc, true);
    expect_has_dest_ssrc(pp, RemoteSsrc, true);

    // Ensure that local peer doesn't try to target report
    // to its own SSRC
    expect_has_dest_ssrc(pp, LocalSsrc, false);
}

// Handle incoming packet from sender without SDES
// (Only SR and XR)
TEST(communicator, missing_sender_sdes) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    send_config.enable_sr_rr = true;
    send_config.enable_xr = true;
    send_config.enable_sdes = false;
    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(send_config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    Config recv_config;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(recv_config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());

    // Check notifications on receiver
    // Notification with empty CNAME should be generated
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, "", SendSsrc, Seed);
}

// Handle incoming packet from receiver without SDES
// (Only RR and XR)
TEST(communicator, missing_receiver_sdes) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(send_config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    Config recv_config;
    recv_config.enable_sr_rr = true;
    recv_config.enable_xr = true;
    recv_config.enable_sdes = false;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(recv_config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver receiver report to sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.total_streams());

    // Check notifications on sender
    // Notification with empty CNAME should be generated
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv_time, "", RecvSsrc,
                       SendSsrc, Seed);
}

// Handle incoming packet from sender without SR
// (Only SDES and XR)
TEST(communicator, missing_sender_sr) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    send_config.enable_sr_rr = false;
    send_config.enable_xr = true;
    send_config.enable_sdes = true;
    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(send_config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    Config recv_config;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(recv_config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());

    // Check notifications on receiver
    // No notifications should be generated
    CHECK_EQUAL(0, recv_part.pending_notifications());
}

// Handle incoming packet from receiver without RR
// (Only SDES and XR)
TEST(communicator, missing_receiver_rr) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(send_config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    Config recv_config;
    recv_config.enable_sr_rr = false;
    recv_config.enable_xr = true;
    recv_config.enable_sdes = true;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(recv_config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver receiver report to sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.total_streams());

    // Check notifications on sender
    // No notifications should be generated
    CHECK_EQUAL(0, send_part.pending_notifications());
}

// Handle incoming packet from sender without XR
// (Only SR and SDES)
TEST(communicator, missing_sender_xr) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    send_config.enable_sr_rr = true;
    send_config.enable_xr = false;
    send_config.enable_sdes = true;
    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(send_config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    Config recv_config;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(recv_config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusOK,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());

    // Check notifications on receiver
    // Notification with zero XR fields should be generated
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send_time, SendCname, SendSsrc,
                       Seed, /* expect_xr = */ false);
}

// Handle incoming packet from receiver without XR
// (Only RR and SDES)
TEST(communicator, missing_receiver_xr) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config send_config;
    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(send_config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    Config recv_config;
    recv_config.enable_sr_rr = true;
    recv_config.enable_xr = false;
    recv_config.enable_sdes = true;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(recv_config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_queue.size());

    // Deliver receiver report to sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK,
                send_comm.process_packet(read_packet(recv_queue), send_time));
    CHECK_EQUAL(1, send_comm.total_streams());

    // Check notifications on sender
    // Notification with zero XR fields should be generated
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv_time, RecvCname, RecvSsrc,
                       SendSsrc, Seed, /* expect_xr = */ false);
}

// Sender report is too large and is split into multiple packets
TEST(communicator, split_sender_report) {
    enum { SendSsrc = 100, RecvSsrc = 200, NumReports = 80, NumPackets = 3 };

    const char* SendCname = "send_cname";

    Config config;
    config.inactivity_timeout = core::Second * 999;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, small_packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Prepare sender
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));

    // Generate reports from multiple receivers to sender to let sender discover them
    for (size_t n_rep = 0; n_rep < NumReports; n_rep++) {
        advance_time(send_time);
        advance_time(recv_time);

        packet::stream_source_t recv_ssrc = RecvSsrc + n_rep;
        char recv_cname[64] = {};
        snprintf(recv_cname, sizeof(recv_cname), "recv_cname%d", (int)recv_ssrc);

        packet::FifoQueue recv_queue;
        MockParticipant recv_part(recv_cname, recv_ssrc, Report_ToAddress);
        Communicator recv_comm(config, recv_part, recv_queue, composer,
                               small_packet_factory, arena);
        LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, recv_cname, recv_ssrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK_EQUAL(1, recv_queue.size());

        // Deliver receiver report to sender
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    send_comm.process_packet(read_packet(recv_queue), send_time));

        // Check notifications on sender
        CHECK_EQUAL(1, send_part.pending_notifications());
        expect_recv_report(send_part.next_recv_notification(), recv_time, recv_cname,
                           recv_ssrc, SendSsrc, Seed);
    }

    advance_time(send_time);

    // Generate sender multi-packet report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(NumReports, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(NumPackets, send_queue.size());

    packet::stream_source_t recv_ssrc = RecvSsrc;
    char recv_cname[64] = {};
    snprintf(recv_cname, sizeof(recv_cname), "recv_cname%d", (int)recv_ssrc);

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(recv_cname, recv_ssrc, Report_ToAddress);
    Communicator recv_comm(config, recv_part, recv_queue, composer, small_packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    // Deliver sender report packets to one of the receivers
    while (send_queue.size() != 0) {
        advance_time(recv_time);

        recv_part.set_recv_report(
            0, make_recv_report(recv_time, recv_cname, recv_ssrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send_queue), recv_time));
    }
    CHECK_EQUAL(1, recv_comm.total_streams());

    // Check notifications on receiver
    CHECK_EQUAL(3, recv_part.pending_notifications());
    for (size_t n_pkt = 0; n_pkt < NumPackets; n_pkt++) {
        expect_send_report(recv_part.next_send_notification(), send_time, SendCname,
                           SendSsrc, Seed);
    }
}

// Receiver report is too large and is split into multiple packets
TEST(communicator, split_receiver_report) {
    enum { SendSsrc = 100, RecvSsrc = 200, NumReports = 15, NumPackets = 5 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;
    config.inactivity_timeout = core::Second * 999;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, small_packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(config, recv_part, recv_queue, composer, small_packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate receiver multi-packet report
    for (size_t n_rep = 0; n_rep < NumReports; n_rep++) {
        recv_part.set_recv_report(
            n_rep,
            make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc + n_rep, Seed));
    }
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
    CHECK_EQUAL(NumReports, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(NumPackets, recv_queue.size());

    // Deliver receiver report packets to sender
    while (recv_queue.size() != 0) {
        advance_time(send_time);

        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    send_comm.process_packet(read_packet(recv_queue), send_time));
    }
    CHECK_EQUAL(1, send_comm.total_streams());

    // Check notifications on sender
    CHECK_EQUAL(NumPackets, send_part.pending_notifications());
    for (size_t n_pkt = 0; n_pkt < NumPackets; n_pkt++) {
        expect_recv_report(send_part.next_recv_notification(), recv_time, RecvCname,
                           RecvSsrc, SendSsrc, Seed);
    }
}

// Bidirectional peer report is too large and is split into multiple packets
TEST(communicator, split_bidirectional_report) {
    enum { LocalSsrc = 100, RemoteSsrc = 200, NumReports = 15, NumPackets = 8 };

    const char* local_cname = "local_cname";

    Config config;
    config.inactivity_timeout = core::Second * 999;

    packet::FifoQueue local_queue;
    MockParticipant local_part(local_cname, LocalSsrc, Report_ToAddress);
    Communicator local_comm(config, local_part, local_queue, composer,
                            small_packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, local_comm.init_status());

    core::nanoseconds_t local_time = 10000000000000000;
    core::nanoseconds_t remote_time = 30000000000000000;

    // Prepare local peer
    local_part.set_send_report(
        make_send_report(local_time, local_cname, LocalSsrc, Seed));

    // Generate reports from multiple remote peers to local peer
    for (size_t n_rep = 0; n_rep < NumReports; n_rep++) {
        advance_time(local_time);
        advance_time(remote_time);

        packet::stream_source_t remote_ssrc = RemoteSsrc + n_rep;
        char remote_cname[64] = {};
        snprintf(remote_cname, sizeof(remote_cname), "remote_cname%d", (int)remote_ssrc);

        packet::FifoQueue remote_queue;
        MockParticipant remote_part(remote_cname, remote_ssrc, Report_ToAddress);
        Communicator remote_comm(config, remote_part, remote_queue, composer,
                                 small_packet_factory, arena);
        LONGS_EQUAL(status::StatusOK, remote_comm.init_status());

        // Generate remote peer report
        remote_part.set_recv_report(
            0, make_recv_report(remote_time, remote_cname, remote_ssrc, LocalSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, remote_comm.generate_reports(remote_time));
        CHECK_EQUAL(1, remote_queue.size());

        // Deliver remote peer report to local peer
        local_part.set_send_report(
            make_send_report(local_time, local_cname, LocalSsrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    local_comm.process_packet(read_packet(remote_queue), local_time));

        // Check notifications on local peer
        CHECK_EQUAL(1, local_part.pending_notifications());
        expect_recv_report(local_part.next_recv_notification(), remote_time, remote_cname,
                           remote_ssrc, LocalSsrc, Seed);
    }

    advance_time(local_time);

    // Generate local peer multi-packet report
    local_part.set_send_report(
        make_send_report(local_time, local_cname, LocalSsrc, Seed));
    for (size_t n_rep = 0; n_rep < NumReports; n_rep++) {
        local_part.set_recv_report(n_rep,
                                   make_recv_report(local_time, local_cname, LocalSsrc,
                                                    RemoteSsrc + n_rep, Seed));
    }
    LONGS_EQUAL(status::StatusOK, local_comm.generate_reports(local_time));
    CHECK_EQUAL(NumReports, local_comm.total_streams());
    CHECK_EQUAL(1, local_comm.total_destinations());
    CHECK_EQUAL(NumPackets, local_queue.size());

    packet::stream_source_t remote_ssrc = RemoteSsrc;
    char remote_cname[64] = {};
    snprintf(remote_cname, sizeof(remote_cname), "remote_cname%d", (int)remote_ssrc);

    packet::FifoQueue remote_queue;
    MockParticipant remote_part(remote_cname, remote_ssrc, Report_ToAddress);
    Communicator remote_comm(config, remote_part, remote_queue, composer,
                             small_packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, remote_comm.init_status());

    // Deliver local peer report packets to one of the remote peers
    while (local_queue.size() != 0) {
        advance_time(remote_time);

        remote_part.set_send_report(
            make_send_report(remote_time, remote_cname, remote_ssrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    remote_comm.process_packet(read_packet(local_queue), remote_time));
    }
    CHECK_EQUAL(1, remote_comm.total_streams());

    // Check notifications on remote peer
    CHECK_EQUAL(NumPackets * 2, remote_part.pending_notifications());
    for (size_t n_pkt = 0; n_pkt < NumPackets; n_pkt++) {
        expect_send_report(remote_part.next_send_notification(), local_time, local_cname,
                           LocalSsrc, Seed);
        expect_recv_report(remote_part.next_recv_notification(), local_time, local_cname,
                           LocalSsrc, RemoteSsrc, Seed);
    }
}

// Tell sender to use specific destination report address
TEST(communicator, report_to_address_sender) {
    enum { SendSsrc = 11, Recv1Ssrc = 22, Recv2Ssrc = 33 };

    const char* SendCname = "send_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    const address::SocketAddr send_dest_addr = make_address(111);

    const address::SocketAddr recv1_addr = make_address(222);
    const address::SocketAddr recv2_addr = make_address(333);

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    send_part.set_report_address(send_dest_addr);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv1_queue;
    MockParticipant recv1_part(Recv1Cname, Recv1Ssrc, Report_ToAddress);
    Communicator recv1_comm(config, recv1_part, recv1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv1_comm.init_status());

    packet::FifoQueue recv2_queue;
    MockParticipant recv2_part(Recv2Cname, Recv2Ssrc, Report_ToAddress);
    Communicator recv2_comm(config, recv2_part, recv2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv2_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv1_time = 30000000000000000;
    core::nanoseconds_t recv2_time = 60000000000000000;

    packet::PacketPtr pp;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));

    // Expect single report to configured address
    // Since receivers were not discovered yet, their SSRCs should
    // not be present
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    pp = read_packet(send_queue);
    expect_has_dest_address(pp, send_dest_addr);
    expect_has_orig_ssrc(pp, SendSsrc, true);
    expect_has_dest_ssrc(pp, Recv1Ssrc, false);
    expect_has_dest_ssrc(pp, Recv2Ssrc, false);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate receiver 1 report
    recv1_part.set_recv_report(
        0, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv1_comm.generate_reports(recv1_time));
    CHECK_EQUAL(1, recv1_queue.size());

    // Deliver receiver 1 report to sender
    pp = read_packet(recv1_queue);
    set_src_address(pp, recv1_addr);
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.process_packet(pp, send_time));
    CHECK_EQUAL(1, send_comm.total_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv1_time, Recv1Cname,
                       Recv1Ssrc, SendSsrc, Seed);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));

    // Expect single report to configured address
    // SSRC of one receiver should be present
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    pp = read_packet(send_queue);
    expect_has_dest_address(pp, send_dest_addr);
    expect_has_orig_ssrc(pp, SendSsrc, true);
    expect_has_dest_ssrc(pp, Recv1Ssrc, true);
    expect_has_dest_ssrc(pp, Recv2Ssrc, false);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate receiver 2 report
    recv2_part.set_recv_report(
        0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));
    CHECK_EQUAL(1, recv2_queue.size());

    // Deliver receiver 2 report to sender
    pp = read_packet(recv2_queue);
    set_src_address(pp, recv2_addr);
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.process_packet(pp, send_time));
    CHECK_EQUAL(2, send_comm.total_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv2_time, Recv2Cname,
                       Recv2Ssrc, SendSsrc, Seed);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));

    // Expect single report to configured address
    // SSRC of both receivers should be present
    CHECK_EQUAL(2, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    pp = read_packet(send_queue);
    expect_has_dest_address(pp, send_dest_addr);
    expect_has_orig_ssrc(pp, SendSsrc, true);
    expect_has_dest_ssrc(pp, Recv1Ssrc, true);
    expect_has_dest_ssrc(pp, Recv2Ssrc, true);
}

// Tell receiver to use specific destination report address
TEST(communicator, report_to_address_receiver) {
    enum { RecvSsrc = 11, Send1Ssrc = 22, Send2Ssrc = 33 };

    const char* RecvCname = "recv_cname";

    const address::SocketAddr recv_dest_addr = make_address(111);

    Config config;

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    recv_part.set_report_address(recv_dest_addr);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t recv_time = 10000000000000000;

    packet::PacketPtr pp;

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    recv_part.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));

    // Expect single report to configured address
    // SSRC of both senders should be present
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    pp = read_packet(recv_queue);
    expect_has_dest_address(pp, recv_dest_addr);
    expect_has_orig_ssrc(pp, RecvSsrc, true);
    expect_has_dest_ssrc(pp, Send1Ssrc, true);
    expect_has_dest_ssrc(pp, Send2Ssrc, true);
}

// Tell sender to deliver reports back to each participant, instead
// of using single destination address for all reports
TEST(communicator, report_back_sender) {
    enum { SendSsrc = 11, Recv1Ssrc = 22, Recv2Ssrc = 33 };

    const char* SendCname = "send_cname";
    const char* Recv1Cname = "recv1_cname";
    const char* Recv2Cname = "recv2_cname";

    const address::SocketAddr recv1_addr = make_address(111);
    const address::SocketAddr recv2_addr = make_address(222);

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_Back);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv1_queue;
    MockParticipant recv1_part(Recv1Cname, Recv1Ssrc, Report_ToAddress);
    Communicator recv1_comm(config, recv1_part, recv1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv1_comm.init_status());

    packet::FifoQueue recv2_queue;
    MockParticipant recv2_part(Recv2Cname, Recv2Ssrc, Report_ToAddress);
    Communicator recv2_comm(config, recv2_part, recv2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, recv2_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv1_time = 30000000000000000;
    core::nanoseconds_t recv2_time = 60000000000000000;

    packet::PacketPtr pp;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));

    // Expect no reports generated because no reports were received yet
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(0, send_comm.total_destinations());
    CHECK_EQUAL(0, send_queue.size());

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate receiver 1 report
    recv1_part.set_recv_report(
        0, make_recv_report(recv1_time, Recv1Cname, Recv1Ssrc, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv1_comm.generate_reports(recv1_time));
    CHECK_EQUAL(1, recv1_queue.size());

    // Deliver receiver 1 report to sender
    pp = read_packet(recv1_queue);
    set_src_address(pp, recv1_addr);
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.process_packet(pp, send_time));
    CHECK_EQUAL(1, send_comm.total_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv1_time, Recv1Cname,
                       Recv1Ssrc, SendSsrc, Seed);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));

    // Expect one report to receiver 1
    CHECK_EQUAL(1, send_comm.total_streams());
    CHECK_EQUAL(1, send_comm.total_destinations());
    CHECK_EQUAL(1, send_queue.size());

    pp = read_packet(send_queue);
    expect_has_dest_address(pp, recv1_addr);
    expect_has_orig_ssrc(pp, SendSsrc, true);
    expect_has_dest_ssrc(pp, Recv1Ssrc, true);
    expect_has_dest_ssrc(pp, Recv2Ssrc, false);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate receiver 2 report
    recv2_part.set_recv_report(
        0, make_recv_report(recv2_time, Recv2Cname, Recv2Ssrc, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv2_comm.generate_reports(recv2_time));
    CHECK_EQUAL(1, recv2_queue.size());

    // Deliver receiver 2 report to sender
    pp = read_packet(recv2_queue);
    set_src_address(pp, recv2_addr);
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.process_packet(pp, send_time));
    CHECK_EQUAL(2, send_comm.total_streams());

    // Check notifications on sender
    CHECK_EQUAL(1, send_part.pending_notifications());
    expect_recv_report(send_part.next_recv_notification(), recv2_time, Recv2Cname,
                       Recv2Ssrc, SendSsrc, Seed);

    advance_time(send_time);
    advance_time(recv1_time);
    advance_time(recv2_time);

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));

    // Expect two reports: to receiver 1 and to receiver 2
    CHECK_EQUAL(2, send_comm.total_streams());
    CHECK_EQUAL(2, send_comm.total_destinations());
    CHECK_EQUAL(2, send_queue.size());

    pp = read_packet(send_queue);
    expect_has_dest_address(pp, recv1_addr);
    expect_has_orig_ssrc(pp, SendSsrc, true);
    expect_has_dest_ssrc(pp, Recv1Ssrc, true);
    expect_has_dest_ssrc(pp, Recv2Ssrc, false);

    pp = read_packet(send_queue);
    expect_has_dest_address(pp, recv2_addr);
    expect_has_orig_ssrc(pp, SendSsrc, true);
    expect_has_dest_ssrc(pp, Recv1Ssrc, false);
    expect_has_dest_ssrc(pp, Recv2Ssrc, true);
}

// Tell receiver to deliver reports back to each participant, instead
// of using single destination address for all reports
TEST(communicator, report_back_receiver) {
    enum { RecvSsrc = 11, Send1Ssrc = 22, Send2Ssrc = 33 };

    const char* RecvCname = "recv_cname";
    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";

    const address::SocketAddr send1_addr = make_address(111);
    const address::SocketAddr send2_addr = make_address(222);

    Config config;

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, Send1Ssrc, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    packet::FifoQueue send2_queue;
    MockParticipant send2_part(Send2Cname, Send2Ssrc, Report_ToAddress);
    Communicator send2_comm(config, send2_part, send2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    core::nanoseconds_t recv_time = 10000000000000000;
    core::nanoseconds_t send1_time = 30000000000000000;
    core::nanoseconds_t send2_time = 60000000000000000;

    packet::PacketPtr pp;

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    recv_part.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));

    // Expect no reports generated because no reports were received yet
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(0, recv_comm.total_destinations());
    CHECK_EQUAL(0, recv_queue.size());

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate sender 1 report
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender 1 report to receiver
    pp = read_packet(send1_queue);
    set_src_address(pp, send1_addr);
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    recv_part.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send1_time, Send1Cname,
                       Send1Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    recv_part.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));

    // Expect one report to sender 1
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(1, recv_comm.total_destinations());
    CHECK_EQUAL(1, recv_queue.size());

    pp = read_packet(recv_queue);
    expect_has_dest_address(pp, send1_addr);
    expect_has_orig_ssrc(pp, RecvSsrc, true);
    expect_has_dest_ssrc(pp, Send1Ssrc, true);
    expect_has_dest_ssrc(pp, Send2Ssrc, false);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate sender 2 report
    send2_part.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender 2 report to receiver
    pp = read_packet(send2_queue);
    set_src_address(pp, send2_addr);
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    recv_part.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send2_time, Send2Cname,
                       Send2Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    recv_part.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));

    // Expect two reports: to sender 1 and to sender 2
    CHECK_EQUAL(2, recv_comm.total_streams());
    CHECK_EQUAL(2, recv_comm.total_destinations());
    CHECK_EQUAL(2, recv_queue.size());

    pp = read_packet(recv_queue);
    expect_has_dest_address(pp, send1_addr);
    expect_has_orig_ssrc(pp, RecvSsrc, true);
    expect_has_dest_ssrc(pp, Send1Ssrc, true);
    expect_has_dest_ssrc(pp, Send2Ssrc, false);

    pp = read_packet(recv_queue);
    expect_has_dest_address(pp, send2_addr);
    expect_has_orig_ssrc(pp, RecvSsrc, true);
    expect_has_dest_ssrc(pp, Send1Ssrc, false);
    expect_has_dest_ssrc(pp, Send2Ssrc, true);
}

// Same as above, but some participants have same address, so there should be
// a single report for those of them
TEST(communicator, report_back_combine_reports) {
    enum { RecvSsrc = 11, Send1Ssrc = 22, Send2Ssrc = 33, Send3Ssrc = 44 };

    const char* RecvCname = "recv_cname";
    const char* Send1Cname = "send1_cname";
    const char* Send2Cname = "send2_cname";
    const char* Send3Cname = "send3_cname";

    const address::SocketAddr send1_addr = make_address(111);
    const address::SocketAddr send2_addr = send1_addr;
    const address::SocketAddr send3_addr = make_address(222);

    Config config;

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    packet::FifoQueue send1_queue;
    MockParticipant send1_part(Send1Cname, Send1Ssrc, Report_ToAddress);
    Communicator send1_comm(config, send1_part, send1_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send1_comm.init_status());

    packet::FifoQueue send2_queue;
    MockParticipant send2_part(Send2Cname, Send2Ssrc, Report_ToAddress);
    Communicator send2_comm(config, send2_part, send2_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send2_comm.init_status());

    packet::FifoQueue send3_queue;
    MockParticipant send3_part(Send3Cname, Send3Ssrc, Report_ToAddress);
    Communicator send3_comm(config, send3_part, send3_queue, composer, packet_factory,
                            arena);
    LONGS_EQUAL(status::StatusOK, send3_comm.init_status());

    core::nanoseconds_t recv_time = 10000000000000000;
    core::nanoseconds_t send1_time = 30000000000000000;
    core::nanoseconds_t send2_time = 60000000000000000;
    core::nanoseconds_t send3_time = 90000000000000000;

    packet::PacketPtr pp;

    // Generate sender 1 report
    send1_part.set_send_report(make_send_report(send1_time, Send1Cname, Send1Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, send1_comm.generate_reports(send1_time));
    CHECK_EQUAL(1, send1_queue.size());

    // Deliver sender 1 report to receiver
    pp = read_packet(send1_queue);
    set_src_address(pp, send1_addr);
    LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send1_time, Send1Cname,
                       Send1Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);
    advance_time(send3_time);

    // Generate sender 2 report
    send2_part.set_send_report(make_send_report(send2_time, Send2Cname, Send2Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, send2_comm.generate_reports(send2_time));
    CHECK_EQUAL(1, send2_queue.size());

    // Deliver sender 2 report to receiver
    pp = read_packet(send2_queue);
    set_src_address(pp, send2_addr);
    LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));
    CHECK_EQUAL(2, recv_comm.total_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send2_time, Send2Cname,
                       Send2Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);
    advance_time(send3_time);

    // Generate sender 3 report
    send3_part.set_send_report(make_send_report(send3_time, Send3Cname, Send3Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, send3_comm.generate_reports(send3_time));
    CHECK_EQUAL(1, send3_queue.size());

    // Deliver sender 3 report to receiver
    pp = read_packet(send3_queue);
    set_src_address(pp, send3_addr);
    LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));
    CHECK_EQUAL(3, recv_comm.total_streams());

    // Check notifications on receiver
    CHECK_EQUAL(1, recv_part.pending_notifications());
    expect_send_report(recv_part.next_send_notification(), send3_time, Send3Cname,
                       Send3Ssrc, Seed);

    advance_time(recv_time);
    advance_time(send1_time);
    advance_time(send2_time);
    advance_time(send3_time);

    // Generate receiver report
    recv_part.set_recv_report(
        0, make_recv_report(recv_time, RecvCname, RecvSsrc, Send1Ssrc, Seed));
    recv_part.set_recv_report(
        1, make_recv_report(recv_time, RecvCname, RecvSsrc, Send2Ssrc, Seed));
    recv_part.set_recv_report(
        2, make_recv_report(recv_time, RecvCname, RecvSsrc, Send3Ssrc, Seed));
    LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));

    // Expect two reports: to senders 1 & 2 and to sender 3
    CHECK_EQUAL(3, recv_comm.total_streams());
    CHECK_EQUAL(2, recv_comm.total_destinations());
    CHECK_EQUAL(2, recv_queue.size());

    pp = read_packet(recv_queue);
    expect_has_dest_address(pp, send1_addr);
    expect_has_orig_ssrc(pp, RecvSsrc, true);
    expect_has_dest_ssrc(pp, Send1Ssrc, true);
    expect_has_dest_ssrc(pp, Send2Ssrc, true);
    expect_has_dest_ssrc(pp, Send3Ssrc, false);

    pp = read_packet(recv_queue);
    expect_has_dest_address(pp, send3_addr);
    expect_has_orig_ssrc(pp, RecvSsrc, true);
    expect_has_dest_ssrc(pp, Send1Ssrc, false);
    expect_has_dest_ssrc(pp, Send2Ssrc, false);
    expect_has_dest_ssrc(pp, Send3Ssrc, true);
}

// Same as above, but reports to same address are also split into multiple packets
// because they're too big
TEST(communicator, report_back_split_reports) {
    enum { LocalSsrc = 100, NumGroups = 2, PeersPerGroup = 20, PacketsPerGroup = 5 };

    const char* local_cname = "local_cname";

    const packet::stream_source_t group_ssrc[NumGroups] = {
        200,
        300,
    };

    const address::SocketAddr group_addr[NumGroups] = {
        make_address(111),
        make_address(222),
    };

    Config config;
    config.inactivity_timeout = core::Second * 999;

    packet::FifoQueue local_queue;
    MockParticipant local_part(local_cname, LocalSsrc, Report_Back);
    Communicator local_comm(config, local_part, local_queue, composer,
                            small_packet_factory, arena);
    LONGS_EQUAL(status::StatusOK, local_comm.init_status());

    core::nanoseconds_t local_time = 10000000000000000;
    core::nanoseconds_t remote_time = 30000000000000000;

    // Generate reports from two groups of peers to local peer
    for (size_t n_grp = 0; n_grp < NumGroups; n_grp++) {
        for (size_t n_peer = 0; n_peer < PeersPerGroup; n_peer++) {
            advance_time(local_time);
            advance_time(remote_time);

            packet::stream_source_t remote_ssrc = group_ssrc[n_grp] + n_peer;
            char remote_cname[64] = {};
            snprintf(remote_cname, sizeof(remote_cname), "remote_cname%d",
                     (int)remote_ssrc);

            packet::FifoQueue remote_queue;
            MockParticipant remote_part(remote_cname, remote_ssrc, Report_ToAddress);
            Communicator remote_comm(config, remote_part, remote_queue, composer,
                                     small_packet_factory, arena);
            LONGS_EQUAL(status::StatusOK, remote_comm.init_status());

            // Generate remote peer report
            remote_part.set_send_report(
                make_send_report(remote_time, remote_cname, remote_ssrc, Seed));
            LONGS_EQUAL(status::StatusOK, remote_comm.generate_reports(remote_time));
            CHECK_EQUAL(1, remote_queue.size());

            // Deliver remote peer report to local peer
            packet::PacketPtr pp = read_packet(remote_queue);
            set_src_address(pp, group_addr[n_grp]);
            LONGS_EQUAL(status::StatusOK, local_comm.process_packet(pp, local_time));

            // Check notifications on local peer
            CHECK_EQUAL(1, local_part.pending_notifications());
            expect_send_report(local_part.next_send_notification(), remote_time,
                               remote_cname, remote_ssrc, Seed);
        }
    }

    advance_time(local_time);

    // Generate local peer multi-packet multi-address report
    for (size_t n_grp = 0; n_grp < NumGroups; n_grp++) {
        for (size_t n_peer = 0; n_peer < PeersPerGroup; n_peer++) {
            local_part.set_recv_report(
                n_grp * PeersPerGroup + n_peer,
                make_recv_report(local_time, local_cname, LocalSsrc,
                                 group_ssrc[n_grp] + n_peer, Seed));
        }
    }

    LONGS_EQUAL(status::StatusOK, local_comm.generate_reports(local_time));
    CHECK_EQUAL(PeersPerGroup * NumGroups, local_comm.total_streams());
    CHECK_EQUAL(NumGroups, local_comm.total_destinations());
    CHECK_EQUAL(PacketsPerGroup * NumGroups, local_queue.size());

    // Check packets
    for (size_t n_grp = 0; n_grp < NumGroups; n_grp++) {
        for (size_t n_pkt = 0; n_pkt < PacketsPerGroup; n_pkt++) {
            packet::PacketPtr pp = read_packet(local_queue);
            expect_has_dest_address(pp, group_addr[n_grp]);
            expect_has_orig_ssrc(pp, LocalSsrc, true);

            for (size_t n_peer = 0; n_peer < PeersPerGroup; n_peer++) {
                const bool peer_present_in_pkt =
                    n_pkt == n_peer / (PeersPerGroup / PacketsPerGroup);

                expect_has_dest_ssrc(pp, group_ssrc[n_grp] + n_peer, peer_present_in_pkt);
            }

            for (size_t other_grp = 0; other_grp < NumGroups; other_grp++) {
                if (other_grp != n_grp) {
                    for (size_t n_peer = 0; n_peer < PeersPerGroup; n_peer++) {
                        expect_has_dest_ssrc(pp, group_ssrc[other_grp] + n_peer, false);
                    }
                }
            }
        }
    }

    CHECK_EQUAL(0, local_queue.size());
}

// Check how communicator computes RTT and clock offset
TEST(communicator, rtt) {
    enum { SendSsrc = 11, RecvSsrc = 22, NumIters = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    const core::nanoseconds_t SendStartTime = 10000000000000000;
    const core::nanoseconds_t RecvStartTime = 30000000000000000;

    const core::nanoseconds_t ReportInterval = 500 * core::Millisecond;
    const core::nanoseconds_t Rtt = 200 * core::Millisecond;

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = SendStartTime;
    core::nanoseconds_t recv_time = RecvStartTime;

    for (int iter = 0; iter < NumIters; iter++) {
        // Generate sender report
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK_EQUAL(1, send_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver sender report to receiver
        if (iter != 0) {
            recv_part.set_recv_report(
                0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        }
        LONGS_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send_queue), recv_time));

        {
            // Check metrics on receiver
            const SendReport report = recv_part.next_send_notification();
            if (iter == 0) {
                CHECK(report.rtt == 0);
                CHECK(report.clock_offset == 0);
            } else {
                expect_timestamp("receiver rtt", Rtt, report.rtt, RttEpsilon);
                expect_timestamp("receiver clock_offset", SendStartTime - RecvStartTime,
                                 report.clock_offset, RttEpsilon);

                // Double-check that clock_offset works as documented
                // If receiver adds estimated clock offset to local receiver timestamp, it
                // should get corresponding remote sender timestamp
                expect_timestamp("receiver mapping", send_time,
                                 recv_time + report.clock_offset, RttEpsilon);
            }
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK_EQUAL(1, recv_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver receiver report to sender
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    send_comm.process_packet(read_packet(recv_queue), send_time));

        {
            // Check metrics on sender
            const RecvReport report = send_part.next_recv_notification();

            expect_timestamp("sender rtt", Rtt, report.rtt, RttEpsilon);
            expect_timestamp("sender clock_offset", RecvStartTime - SendStartTime,
                             report.clock_offset, RttEpsilon);

            // Double-check that clock_offset works as documented
            // If sender adds estimated clock offset to local sender timestamp, it
            // should get corresponding remote receiver timestamp
            expect_timestamp("sender mapping", recv_time, send_time + report.clock_offset,
                             RttEpsilon);
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);
    }
}

// Same, but there is a persistent clock drift between sender and receiver
TEST(communicator, rtt_clock_drift) {
    enum { SendSsrc = 11, RecvSsrc = 22, NumIters = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    const core::nanoseconds_t SendStartTime = 10000000000000000;
    const core::nanoseconds_t RecvStartTime = 30000000000000000;

    const core::nanoseconds_t ReportInterval = 500 * core::Millisecond;
    const core::nanoseconds_t Rtt = 200 * core::Millisecond;
    const core::nanoseconds_t Drift = 1 * core::Millisecond;

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = SendStartTime;
    core::nanoseconds_t recv_time = RecvStartTime;

    for (int iter = 0; iter < NumIters; iter++) {
        // Generate sender report
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK_EQUAL(1, send_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver sender report to receiver
        if (iter != 0) {
            recv_part.set_recv_report(
                0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        }
        LONGS_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send_queue), recv_time));

        {
            // Check metrics on receiver
            const SendReport report = recv_part.next_send_notification();
            if (iter != 0) {
                expect_timestamp("receiver rtt", Rtt, report.rtt, RttEpsilon + Drift);
                expect_timestamp("receiver clock_offset",
                                 SendStartTime + (Drift * iter) - RecvStartTime,
                                 report.clock_offset, RttEpsilon + Drift);
            }
        }

        advance_time(send_time, ReportInterval / 2 + Drift);
        advance_time(recv_time, ReportInterval / 2);

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK_EQUAL(1, recv_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver receiver report to sender
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    send_comm.process_packet(read_packet(recv_queue), send_time));

        {
            // Check metrics on sender
            const RecvReport report = send_part.next_recv_notification();

            expect_timestamp("sender rtt", Rtt, report.rtt, RttEpsilon + Drift);
            expect_timestamp("sender clock_offset",
                             RecvStartTime - SendStartTime - (Drift * iter),
                             report.clock_offset, RttEpsilon + Drift);
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);
    }
}

// Same, but there is persistent network jitter
TEST(communicator, rtt_network_jitter) {
    enum { SendSsrc = 11, RecvSsrc = 22, NumIters = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    const core::nanoseconds_t SendStartTime = 10000000000000000;
    const core::nanoseconds_t RecvStartTime = 30000000000000000;

    const core::nanoseconds_t ReportInterval = 500 * core::Millisecond;
    const core::nanoseconds_t Rtt = 200 * core::Millisecond;
    const core::nanoseconds_t Jitter = 30 * core::Millisecond;

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = SendStartTime;
    core::nanoseconds_t recv_time = RecvStartTime;

    for (int iter = 0; iter < NumIters; iter++) {
        // Select pseudo-random jitter for current iteration
        const core::nanoseconds_t iter_jitter =
            (iter % (Jitter / core::Millisecond)) * core::Millisecond - Jitter / 2;

        // Generate sender report
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK_EQUAL(1, send_queue.size());

        advance_time(send_time, Rtt / 2 + iter_jitter);
        advance_time(recv_time, Rtt / 2 + iter_jitter);

        // Deliver sender report to receiver
        if (iter != 0) {
            recv_part.set_recv_report(
                0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        }
        LONGS_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send_queue), recv_time));

        {
            // Check metrics on receiver
            const SendReport report = recv_part.next_send_notification();
            if (iter != 0) {
                expect_timestamp("receiver rtt", Rtt, report.rtt, RttEpsilon + Jitter);
                expect_timestamp("receiver clock_offset", SendStartTime - RecvStartTime,
                                 report.clock_offset, RttEpsilon + Jitter);
            }
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK_EQUAL(1, recv_queue.size());

        advance_time(send_time, Rtt / 2 + iter_jitter);
        advance_time(recv_time, Rtt / 2 + iter_jitter);

        // Deliver receiver report to sender
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    send_comm.process_packet(read_packet(recv_queue), send_time));

        {
            // Check metrics on sender
            const RecvReport report = send_part.next_recv_notification();

            expect_timestamp("sender rtt", Rtt, report.rtt, RttEpsilon + Jitter);
            expect_timestamp("sender clock_offset", RecvStartTime - SendStartTime,
                             report.clock_offset, RttEpsilon + Jitter);
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);
    }
}

// Same, but there are occasional packet losses
TEST(communicator, rtt_network_losses) {
    enum { SendSsrc = 11, RecvSsrc = 22, NumIters = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    const core::nanoseconds_t SendStartTime = 10000000000000000;
    const core::nanoseconds_t RecvStartTime = 30000000000000000;

    const core::nanoseconds_t ReportInterval = 500 * core::Millisecond;
    const core::nanoseconds_t Rtt = 200 * core::Millisecond;
    const int DropFreq = 9;

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = SendStartTime;
    core::nanoseconds_t recv_time = RecvStartTime;

    for (int iter = 0; iter < NumIters; iter++) {
        // Decide whether to loss packets on this iteration
        const bool loss_send_report = (iter + 3) % DropFreq == 0;
        const bool loss_recv_report = (iter + 5) % DropFreq == 0;

        packet::PacketPtr pp;

        // Generate sender report
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK_EQUAL(1, send_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver sender report to receiver
        pp = read_packet(send_queue);

        if (!loss_send_report) {
            if (iter != 0) {
                recv_part.set_recv_report(
                    0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
            }
            LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));

            // Check metrics on receiver
            const SendReport report = recv_part.next_send_notification();
            if (iter != 0) {
                expect_timestamp("receiver rtt", Rtt, report.rtt, RttEpsilon);
                expect_timestamp("receiver clock_offset", SendStartTime - RecvStartTime,
                                 report.clock_offset, RttEpsilon);
            }
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK_EQUAL(1, recv_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver receiver report to sender
        pp = read_packet(recv_queue);

        if (!loss_recv_report) {
            send_part.set_send_report(
                make_send_report(send_time, SendCname, SendSsrc, Seed));
            LONGS_EQUAL(status::StatusOK, send_comm.process_packet(pp, send_time));

            // Check metrics on sender
            const RecvReport report = send_part.next_recv_notification();

            expect_timestamp("sender rtt", Rtt, report.rtt, RttEpsilon);
            expect_timestamp("sender clock_offset", RecvStartTime - SendStartTime,
                             report.clock_offset, RttEpsilon);
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);
    }
}

// Same, but there are occasional burst delays
TEST(communicator, rtt_network_delays) {
    enum { SendSsrc = 11, RecvSsrc = 22, NumIters = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    const core::nanoseconds_t SendStartTime = 10000000000000000;
    const core::nanoseconds_t RecvStartTime = 30000000000000000;

    const core::nanoseconds_t ReportInterval = 500 * core::Millisecond;
    const core::nanoseconds_t Rtt = 200 * core::Millisecond;
    const int DelayFreq = 9;
    const int DelayBurst = 3;
    const core::nanoseconds_t MaxDelay = (ReportInterval + Rtt) * DelayBurst;

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = SendStartTime;
    core::nanoseconds_t recv_time = RecvStartTime;

    int send_delay_countdown = 0;
    int recv_delay_countdown = 0;

    for (int iter = 0; iter < NumIters; iter++) {
        // Decide whether to delay packets from this iteration to next one
        const bool start_send_delay = (iter + 3) % DelayFreq == 0;
        const bool start_recv_delay = (iter + 5) % DelayFreq == 0;

        // Generate sender report
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK(send_queue.size() > 0);

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver sender reports to receiver
        if (!start_send_delay && send_delay_countdown == 0) {
            while (send_queue.size() != 0) {
                packet::PacketPtr pp = read_packet(send_queue);

                if (iter != 0) {
                    recv_part.set_recv_report(
                        0,
                        make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
                }
                LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));

                // Check metrics on receiver
                const SendReport report = recv_part.next_send_notification();
                if (iter != 0) {
                    expect_timestamp("receiver rtt", Rtt, report.rtt,
                                     RttEpsilon + MaxDelay);
                    expect_timestamp("receiver clock_offset",
                                     SendStartTime - RecvStartTime, report.clock_offset,
                                     RttEpsilon + MaxDelay);
                }
            }
        } else {
            if (start_send_delay) {
                send_delay_countdown = DelayBurst;
            }
            send_delay_countdown--;
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK(recv_queue.size() > 0);

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver receiver reports to sender
        if (!start_recv_delay && recv_delay_countdown == 0) {
            while (recv_queue.size() != 0) {
                packet::PacketPtr pp = read_packet(recv_queue);

                send_part.set_send_report(
                    make_send_report(send_time, SendCname, SendSsrc, Seed));
                LONGS_EQUAL(status::StatusOK, send_comm.process_packet(pp, send_time));

                // Check metrics on sender
                const RecvReport report = send_part.next_recv_notification();

                expect_timestamp("sender rtt", Rtt, report.rtt, RttEpsilon + MaxDelay);
                expect_timestamp("sender clock_offset", RecvStartTime - SendStartTime,
                                 report.clock_offset, RttEpsilon + MaxDelay);
            }
        } else {
            if (start_recv_delay) {
                recv_delay_countdown = DelayBurst;
            }
            recv_delay_countdown--;
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);
    }
}

// Same, but there are occasional burst reorders
TEST(communicator, rtt_network_reordering) {
    enum { SendSsrc = 11, RecvSsrc = 22, NumIters = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    const core::nanoseconds_t SendStartTime = 10000000000000000;
    const core::nanoseconds_t RecvStartTime = 30000000000000000;

    const core::nanoseconds_t ReportInterval = 500 * core::Millisecond;
    const core::nanoseconds_t Rtt = 200 * core::Millisecond;
    const int ReorderFreq = 9;
    const int ReorderBurst = 3;
    const core::nanoseconds_t MaxDelay = (ReportInterval + Rtt) * ReorderBurst;

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = SendStartTime;
    core::nanoseconds_t recv_time = RecvStartTime;

    core::List<packet::Packet> send_packet_stash;
    core::List<packet::Packet> recv_packet_stash;

    int send_reorder_countdown = 0;
    int recv_reorder_countdown = 0;

    for (int iter = 0; iter < NumIters; iter++) {
        // Decide whether to reorder packets from this iteration with next one
        const bool start_send_reorder = (iter + 3) % ReorderFreq == 0;
        const bool start_recv_reorder = (iter + 5) % ReorderFreq == 0;

        // Generate sender report
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK_EQUAL(1, send_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver sender reports to receiver
        if (!start_send_reorder && send_reorder_countdown == 0) {
            // Enqueue stashed packets (if any) in reverse order
            while (packet::PacketPtr pp = send_packet_stash.back()) {
                send_packet_stash.remove(*pp);
                LONGS_EQUAL(status::StatusOK, send_queue.write(pp));
            }

            while (send_queue.size() != 0) {
                packet::PacketPtr pp = read_packet(send_queue);

                if (iter != 0) {
                    recv_part.set_recv_report(
                        0,
                        make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
                }
                LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));

                // Check metrics on receiver
                const SendReport report = recv_part.next_send_notification();
                if (iter != 0) {
                    expect_timestamp("receiver rtt", Rtt, report.rtt,
                                     RttEpsilon + MaxDelay);
                    expect_timestamp("receiver clock_offset",
                                     SendStartTime - RecvStartTime, report.clock_offset,
                                     RttEpsilon + MaxDelay);
                }
            }
        } else {
            if (start_send_reorder) {
                send_reorder_countdown = ReorderBurst;
            }
            send_reorder_countdown--;
            packet::PacketPtr pp = read_packet(send_queue);
            send_packet_stash.push_back(*pp);
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK_EQUAL(1, recv_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver receiver reports to sender
        if (!start_recv_reorder && recv_reorder_countdown == 0) {
            // Enqueue stashed packets (if any) in reverse order
            while (packet::PacketPtr pp = recv_packet_stash.back()) {
                recv_packet_stash.remove(*pp);
                LONGS_EQUAL(status::StatusOK, recv_queue.write(pp));
            }

            while (recv_queue.size() != 0) {
                packet::PacketPtr pp = read_packet(recv_queue);

                send_part.set_send_report(
                    make_send_report(send_time, SendCname, SendSsrc, Seed));
                LONGS_EQUAL(status::StatusOK, send_comm.process_packet(pp, send_time));

                // Check metrics on sender
                const RecvReport report = send_part.next_recv_notification();

                expect_timestamp("sender rtt", Rtt, report.rtt, RttEpsilon + MaxDelay);
                expect_timestamp("sender clock_offset", RecvStartTime - SendStartTime,
                                 report.clock_offset, RttEpsilon + MaxDelay);
            }
        } else {
            if (start_recv_reorder) {
                recv_reorder_countdown = ReorderBurst;
            }
            recv_reorder_countdown--;
            packet::PacketPtr pp = read_packet(recv_queue);
            recv_packet_stash.push_back(*pp);
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);
    }
}

// Same, but there are occasional duplicates
TEST(communicator, rtt_network_duplicates) {
    enum { SendSsrc = 11, RecvSsrc = 22, NumIters = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    const core::nanoseconds_t SendStartTime = 10000000000000000;
    const core::nanoseconds_t RecvStartTime = 30000000000000000;

    const core::nanoseconds_t ReportInterval = 500 * core::Millisecond;
    const core::nanoseconds_t Rtt = 200 * core::Millisecond;
    const int DupFreq = 9;
    const int DupDelay = 3;

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = SendStartTime;
    core::nanoseconds_t recv_time = RecvStartTime;

    packet::PacketPtr send_dup_packet;
    packet::PacketPtr recv_dup_packet;

    int send_dup_countdown = 0;
    int recv_dup_countdown = 0;

    for (int iter = 0; iter < NumIters; iter++) {
        // Decide whether to duplicate packets from this iteration to next one
        const bool dup_send_report = (iter + 3) % DupFreq == 0;
        const bool dup_recv_report = (iter + 5) % DupFreq == 0;

        packet::PacketPtr pp;

        // Generate sender report
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK(send_queue.size() > 0);

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Enqueue duplicate packet
        if (send_dup_packet) {
            if (--send_dup_countdown == 0) {
                LONGS_EQUAL(status::StatusOK, send_queue.write(send_dup_packet));
                send_dup_packet = NULL;
            }
        }

        // Deliver sender reports to receiver
        while (send_queue.size() != 0) {
            pp = read_packet(send_queue);

            if (iter != 0) {
                recv_part.set_recv_report(
                    0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
            }
            LONGS_EQUAL(status::StatusOK, recv_comm.process_packet(pp, recv_time));

            // Check metrics on receiver
            const SendReport report = recv_part.next_send_notification();
            if (iter != 0) {
                expect_timestamp("receiver rtt", Rtt, report.rtt, RttEpsilon);
                expect_timestamp("receiver clock_offset", SendStartTime - RecvStartTime,
                                 report.clock_offset, RttEpsilon);
            }
        }

        if (dup_send_report) {
            send_dup_packet = pp;
            send_dup_countdown = DupDelay;
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK(recv_queue.size() > 0);

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Enqueue duplicate packet
        if (recv_dup_packet) {
            if (--recv_dup_countdown == 0) {
                LONGS_EQUAL(status::StatusOK, recv_queue.write(recv_dup_packet));
                recv_dup_packet = NULL;
            }
        }

        // Deliver receiver reports to sender
        while (recv_queue.size() != 0) {
            pp = read_packet(recv_queue);

            send_part.set_send_report(
                make_send_report(send_time, SendCname, SendSsrc, Seed));
            LONGS_EQUAL(status::StatusOK, send_comm.process_packet(pp, send_time));

            // Check metrics on sender
            const RecvReport report = send_part.next_recv_notification();

            expect_timestamp("sender rtt", Rtt, report.rtt, RttEpsilon);
            expect_timestamp("sender clock_offset", RecvStartTime - SendStartTime,
                             report.clock_offset, RttEpsilon);
        }

        if (dup_recv_report) {
            recv_dup_packet = pp;
            recv_dup_countdown = DupDelay;
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);
    }
}

// If XR is disabled, RTT and clock offset should remain zero
TEST(communicator, rtt_missing_xr) {
    enum { SendSsrc = 11, RecvSsrc = 22, NumIters = 200 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    const core::nanoseconds_t SendStartTime = 10000000000000000;
    const core::nanoseconds_t RecvStartTime = 30000000000000000;

    const core::nanoseconds_t ReportInterval = 500 * core::Millisecond;
    const core::nanoseconds_t Rtt = 200 * core::Millisecond;

    Config config;
    config.enable_xr = false;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_Back);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = SendStartTime;
    core::nanoseconds_t recv_time = RecvStartTime;

    for (int iter = 0; iter < NumIters; iter++) {
        // Generate sender report
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK_EQUAL(1, send_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver sender report to receiver
        if (iter != 0) {
            recv_part.set_recv_report(
                0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        }
        LONGS_EQUAL(status::StatusOK,
                    recv_comm.process_packet(read_packet(send_queue), recv_time));

        {
            // Check metrics on receiver
            const SendReport report = recv_part.next_send_notification();
            CHECK(report.rtt == 0);
            CHECK(report.clock_offset == 0);
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);

        // Generate receiver report
        recv_part.set_recv_report(
            0, make_recv_report(recv_time, RecvCname, RecvSsrc, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK, recv_comm.generate_reports(recv_time));
        CHECK_EQUAL(1, recv_queue.size());

        advance_time(send_time, Rtt / 2);
        advance_time(recv_time, Rtt / 2);

        // Deliver receiver report to sender
        send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
        LONGS_EQUAL(status::StatusOK,
                    send_comm.process_packet(read_packet(recv_queue), send_time));

        {
            // Check metrics on sender
            const RecvReport report = send_part.next_recv_notification();
            CHECK(report.rtt == 0);
            CHECK(report.clock_offset == 0);
        }

        advance_time(send_time, ReportInterval / 2);
        advance_time(recv_time, ReportInterval / 2);
    }
}

TEST(communicator, generation_error) {
    enum { Ssrc = 11 };

    const char* Cname = "test_cname";
    Config config;

    { // forward error from arena
        MockArena peer_arena;
        packet::FifoQueue peer_queue;
        MockParticipant peer_part(Cname, Ssrc, Report_ToAddress);
        Communicator peer_comm(config, peer_part, peer_queue, composer, packet_factory,
                               peer_arena);
        LONGS_EQUAL(status::StatusOK, peer_comm.init_status());

        core::nanoseconds_t peer_time = 10000000000000000;

        // Tell arena to fail
        peer_arena.set_fail(true);
        peer_part.set_send_report(make_send_report(peer_time, Cname, Ssrc, Seed));
        for (size_t idx = 0; idx < 50; idx++) {
            // Tell participant to report 50 streams to
            // force allocations from arena
            peer_part.set_recv_report(
                idx, make_recv_report(peer_time, Cname, Ssrc, Ssrc, Seed));
        }

        LONGS_EQUAL(status::StatusNoMem, peer_comm.generate_reports(peer_time));
        CHECK_EQUAL(0, peer_comm.total_streams());
        CHECK_EQUAL(0, peer_queue.size());
    }
    { // forward error from writer
        MockWriter peer_writer(status::StatusDrain);
        MockParticipant peer_part(Cname, Ssrc, Report_ToAddress);
        Communicator peer_comm(config, peer_part, peer_writer, composer, packet_factory,
                               arena);
        LONGS_EQUAL(status::StatusOK, peer_comm.init_status());

        core::nanoseconds_t peer_time = 10000000000000000;

        peer_part.set_send_report(make_send_report(peer_time, Cname, Ssrc, Seed));
        LONGS_EQUAL(status::StatusDrain, peer_comm.generate_reports(peer_time));
        CHECK_EQUAL(0, peer_comm.total_streams());
        CHECK_EQUAL(1, peer_writer.call_count());
    }
    { // buffer factory w/ small buffers
        packet::FifoQueue peer_queue;
        MockParticipant peer_part(Cname, Ssrc, Report_ToAddress);
        // factory creates unreasonably small buffers
        Communicator peer_comm(config, peer_part, peer_queue, composer,
                               tiny_packet_factory, arena);
        LONGS_EQUAL(status::StatusOK, peer_comm.init_status());

        core::nanoseconds_t peer_time = 10000000000000000;

        peer_part.set_send_report(make_send_report(peer_time, Cname, Ssrc, Seed));
        LONGS_EQUAL(status::StatusNoMem, peer_comm.generate_reports(peer_time));
        CHECK_EQUAL(0, peer_comm.total_streams());
        CHECK_EQUAL(0, peer_queue.size());
    }
}

TEST(communicator, processing_error) {
    enum { RecvSsrc = 11 };

    const char* RecvCname = "recv_cname";

    Config config;
    config.inactivity_timeout = core::Second * 999;

    MockArena recv_arena;
    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           recv_arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

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

        packet::FifoQueue send_queue;
        MockParticipant send_part(send_cname, send_ssrc, Report_ToAddress);
        Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                               arena);
        LONGS_EQUAL(status::StatusOK, send_comm.init_status());

        // Generate sender report
        send_part.set_send_report(
            make_send_report(send_time, send_cname, send_ssrc, Seed));
        LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
        CHECK_EQUAL(0, send_comm.total_streams());
        CHECK_EQUAL(1, send_queue.size());

        // Deliver sender report to receiver
        const status::StatusCode status =
            recv_comm.process_packet(read_packet(send_queue), recv_time);

        if (status == status::StatusOK) {
            // Check notifications on receiver
            CHECK_EQUAL(n_reports, recv_comm.total_streams());
            CHECK_EQUAL(1, recv_part.pending_notifications());
            expect_send_report(recv_part.next_send_notification(), send_time, send_cname,
                               send_ssrc, Seed);

            // Repeat until failure.
            // First few iterations will succeed because arena is not used
            // until pre-allocated capacity is full.
            continue;
        } else {
            // Finally allocation failed and reported.
            LONGS_EQUAL(status::StatusNoMem, status);
            CHECK_EQUAL(n_reports - 1, recv_comm.total_streams());
            CHECK_EQUAL(0, recv_part.pending_notifications());
            break;
        }
    }
}

TEST(communicator, notification_error) {
    enum { SendSsrc = 11, RecvSsrc = 22 };

    const char* SendCname = "send_cname";
    const char* RecvCname = "recv_cname";

    Config config;

    packet::FifoQueue send_queue;
    MockParticipant send_part(SendCname, SendSsrc, Report_ToAddress);
    Communicator send_comm(config, send_part, send_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, send_comm.init_status());

    packet::FifoQueue recv_queue;
    MockParticipant recv_part(RecvCname, RecvSsrc, Report_ToAddress);
    Communicator recv_comm(config, recv_part, recv_queue, composer, packet_factory,
                           arena);
    LONGS_EQUAL(status::StatusOK, recv_comm.init_status());

    core::nanoseconds_t send_time = 10000000000000000;
    core::nanoseconds_t recv_time = 30000000000000000;

    // Generate sender report
    send_part.set_send_report(make_send_report(send_time, SendCname, SendSsrc, Seed));
    LONGS_EQUAL(status::StatusOK, send_comm.generate_reports(send_time));
    CHECK_EQUAL(0, send_comm.total_streams());
    CHECK_EQUAL(1, send_queue.size());

    // Tell receiver to return error from notification handler
    recv_part.set_status(status::StatusDrain);

    // Deliver sender report to receiver
    LONGS_EQUAL(status::StatusDrain,
                recv_comm.process_packet(read_packet(send_queue), recv_time));
    CHECK_EQUAL(1, recv_comm.total_streams());

    // Check notifications on receiver
    CHECK_EQUAL(0, recv_part.pending_notifications());
}

} // namespace rtcp
} // namespace roc
