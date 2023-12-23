/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_session_group.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_rtcp/communicator.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace pipeline {

ReceiverSessionGroup::ReceiverSessionGroup(
    const ReceiverConfig& receiver_config,
    ReceiverState& receiver_state,
    audio::Mixer& mixer,
    const rtp::EncodingMap& encoding_map,
    packet::PacketFactory& packet_factory,
    core::BufferFactory<uint8_t>& byte_buffer_factory,
    core::BufferFactory<audio::sample_t>& sample_buffer_factory,
    core::IArena& arena)
    : arena_(arena)
    , packet_factory_(packet_factory)
    , byte_buffer_factory_(byte_buffer_factory)
    , sample_buffer_factory_(sample_buffer_factory)
    , encoding_map_(encoding_map)
    , mixer_(mixer)
    , receiver_state_(receiver_state)
    , receiver_config_(receiver_config)
    , valid_(false) {
    identity_.reset(new (identity_) rtp::Identity());
    if (!identity_ || !identity_->is_valid()) {
        return;
    }

    valid_ = true;
}

ReceiverSessionGroup::~ReceiverSessionGroup() {
    remove_all_sessions_();
}

bool ReceiverSessionGroup::is_valid() const {
    return valid_;
}

status::StatusCode ReceiverSessionGroup::route_packet(const packet::PacketPtr& packet,
                                                      core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    if (packet->rtcp()) {
        return route_control_packet_(packet, current_time);
    }

    return route_transport_packet_(packet);
}

core::nanoseconds_t
ReceiverSessionGroup::refresh_sessions(core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    core::SharedPtr<ReceiverSession> curr, next;

    core::nanoseconds_t next_deadline = 0;

    for (curr = sessions_.front(); curr; curr = next) {
        next = sessions_.nextof(*curr);

        core::nanoseconds_t sess_deadline = 0;

        if (!curr->refresh(current_time, &sess_deadline)) {
            // Session ended.
            remove_session_(*curr);
            continue;
        }

        if (sess_deadline != 0) {
            if (next_deadline == 0) {
                next_deadline = sess_deadline;
            } else {
                next_deadline = std::min(next_deadline, sess_deadline);
            }
        }
    }

    return next_deadline;
}

void ReceiverSessionGroup::reclock_sessions(core::nanoseconds_t playback_time) {
    roc_panic_if(!is_valid());

    core::SharedPtr<ReceiverSession> curr, next;

    for (curr = sessions_.front(); curr; curr = next) {
        next = sessions_.nextof(*curr);

        if (!curr->reclock(playback_time)) {
            // Session ended.
            remove_session_(*curr);
        }
    }
}

size_t ReceiverSessionGroup::num_sessions() const {
    roc_panic_if(!is_valid());

    return sessions_.size();
}

void ReceiverSessionGroup::get_metrics(ReceiverSessionMetrics* metrics,
                                       size_t* metrics_size) const {
    roc_panic_if(!is_valid());

    roc_panic_if_not(metrics);
    roc_panic_if_not(metrics_size);

    *metrics_size = std::min(*metrics_size, sessions_.size());

    size_t n = 0;

    for (core::SharedPtr<ReceiverSession> sess = sessions_.front(); sess;
         sess = sessions_.nextof(*sess)) {
        if (n == *metrics_size) {
            break;
        }
        metrics[n] = sess->get_metrics();
        n++;
    }
}

const char* ReceiverSessionGroup::cname() {
    return identity_->cname();
}

packet::stream_source_t ReceiverSessionGroup::source_id() {
    return identity_->ssrc();
}

void ReceiverSessionGroup::change_source_id() {
    identity_->change_ssrc();
}

size_t ReceiverSessionGroup::num_recv_steams() {
    // TODO(gh-14): query num sessions
    return 0;
}

rtcp::RecvReport
ReceiverSessionGroup::query_recv_stream(size_t recv_stream_index,
                                        core::nanoseconds_t report_time) {
    rtcp::RecvReport report;
    report.receiver_cname = identity_->cname();
    report.receiver_source_id = identity_->ssrc();
    // TODO(gh-14): query session
    report.sender_source_id = 123;
    report.report_timestamp = report_time;
    report.extended_seqnum = 0;
    report.fract_loss = 0;
    report.cum_loss = 0;
    report.jitter = 0;

    return report;
}

void ReceiverSessionGroup::notify_recv_stream(packet::stream_source_t send_source_id,
                                              const rtcp::SendReport& send_report) {
    // TODO(gh-14): match session by SSRC/CNAME
    core::SharedPtr<ReceiverSession> sess;

    for (sess = sessions_.front(); sess; sess = sessions_.nextof(*sess)) {
        sess->process_report(send_report);
    }
}

void ReceiverSessionGroup::halt_recv_stream(packet::stream_source_t send_source_id) {
    // TODO(gh-14): remove session
}

status::StatusCode
ReceiverSessionGroup::route_transport_packet_(const packet::PacketPtr& packet) {
    core::SharedPtr<ReceiverSession> sess;

    for (sess = sessions_.front(); sess; sess = sessions_.nextof(*sess)) {
        const status::StatusCode code = sess->route_packet(packet);
        if (code == status::StatusOK) {
            // TODO(gh-183): hadle StatusNoRoute vs other error
            return code;
        }
    }

    if (!can_create_session_(packet)) {
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    return create_session_(packet);
}

status::StatusCode
ReceiverSessionGroup::route_control_packet_(const packet::PacketPtr& packet,
                                            core::nanoseconds_t current_time) {
    if (!rtcp_composer_) {
        rtcp_composer_.reset(new (rtcp_composer_) rtcp::Composer());
    }

    if (!rtcp_communicator_) {
        rtcp_communicator_.reset(new (rtcp_communicator_) rtcp::Communicator(
            receiver_config_.common.rtcp_config, *this, NULL, *rtcp_composer_,
            packet_factory_, byte_buffer_factory_, arena_));
    }

    if (!rtcp_communicator_->is_valid()) {
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    // This will invoke IReceiverController methods implemented by us.
    return rtcp_communicator_->process_packet(packet, current_time);
}

bool ReceiverSessionGroup::can_create_session_(const packet::PacketPtr& packet) {
    if (packet->flags() & packet::Packet::FlagRepair) {
        roc_log(LogDebug, "session group: ignoring repair packet for unknown session");
        return false;
    }

    return true;
}

status::StatusCode
ReceiverSessionGroup::create_session_(const packet::PacketPtr& packet) {
    if (!packet->udp()) {
        roc_log(LogError,
                "session group: can't create session, unexpected non-udp packet");
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    if (!packet->rtp()) {
        roc_log(LogError,
                "session group: can't create session, unexpected non-rtp packet");
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    const ReceiverSessionConfig sess_config = make_session_config_(packet);

    const address::SocketAddr src_address = packet->udp()->src_addr;
    const address::SocketAddr dst_address = packet->udp()->dst_addr;

    roc_log(LogInfo, "session group: creating session: src_addr=%s dst_addr=%s",
            address::socket_addr_to_str(src_address).c_str(),
            address::socket_addr_to_str(dst_address).c_str());

    core::SharedPtr<ReceiverSession> sess = new (arena_) ReceiverSession(
        sess_config, receiver_config_.common, src_address, encoding_map_, packet_factory_,
        byte_buffer_factory_, sample_buffer_factory_, arena_);

    if (!sess || !sess->is_valid()) {
        roc_log(LogError, "session group: can't create session, initialization failed");
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    const status::StatusCode code = sess->route_packet(packet);
    if (code != status::StatusOK) {
        roc_log(
            LogError,
            "session group: can't create session, can't handle first packet: status=%s",
            status::code_to_str(code));
        // TODO(gh-183): handle and return status
        return status::StatusOK;
    }

    mixer_.add_input(sess->reader());
    sessions_.push_back(*sess);

    receiver_state_.add_sessions(+1);

    return status::StatusOK;
}

void ReceiverSessionGroup::remove_session_(ReceiverSession& sess) {
    roc_log(LogInfo, "session group: removing session");

    mixer_.remove_input(sess.reader());
    sessions_.remove(sess);

    receiver_state_.add_sessions(-1);
}

void ReceiverSessionGroup::remove_all_sessions_() {
    roc_log(LogDebug, "session group: removing all sessions");

    while (!sessions_.is_empty()) {
        remove_session_(*sessions_.back());
    }
}

ReceiverSessionConfig
ReceiverSessionGroup::make_session_config_(const packet::PacketPtr& packet) const {
    ReceiverSessionConfig config = receiver_config_.default_session;

    packet::RTP* rtp = packet->rtp();
    if (rtp) {
        config.payload_type = rtp->payload_type;
    }

    packet::FEC* fec = packet->fec();
    if (fec) {
        config.fec_decoder.scheme = fec->fec_scheme;
    }

    return config;
}

} // namespace pipeline
} // namespace roc
