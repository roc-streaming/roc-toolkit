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
#include "roc_status/code_to_str.h"

namespace roc {
namespace pipeline {

ReceiverSessionGroup::ReceiverSessionGroup(
    const ReceiverConfig& receiver_config,
    StateTracker& state_tracker,
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
    , state_tracker_(state_tracker)
    , receiver_config_(receiver_config)
    , session_router_(arena)
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

bool ReceiverSessionGroup::create_control_pipeline(ReceiverEndpoint* control_endpoint) {
    roc_panic_if(!is_valid());

    roc_panic_if(!control_endpoint);
    roc_panic_if(!control_endpoint->outbound_composer()
                 || !control_endpoint->outbound_writer());
    roc_panic_if(rtcp_communicator_);

    rtcp_communicator_.reset(new (rtcp_communicator_) rtcp::Communicator(
        receiver_config_.common.rtcp_config, *this, *control_endpoint->outbound_writer(),
        *control_endpoint->outbound_composer(), packet_factory_, byte_buffer_factory_,
        arena_));
    if (!rtcp_communicator_ || !rtcp_communicator_->is_valid()) {
        rtcp_communicator_.reset();
        return false;
    }

    return true;
}

status::StatusCode ReceiverSessionGroup::route_packet(const packet::PacketPtr& packet,
                                                      core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    if (packet->has_flags(packet::Packet::FlagControl)) {
        return route_control_packet_(packet, current_time);
    }

    return route_transport_packet_(packet);
}

core::nanoseconds_t
ReceiverSessionGroup::refresh_sessions(core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    core::SharedPtr<ReceiverSession> curr, next;

    core::nanoseconds_t next_deadline = 0;

    if (rtcp_communicator_) {
        // This will invoke IParticipant methods implemented by us,
        // in particular query_recv_streams().
        const status::StatusCode code =
            rtcp_communicator_->generate_reports(current_time);
        // TODO(gh-183): forward status
        roc_panic_if(code != status::StatusOK);

        next_deadline = rtcp_communicator_->generation_deadline(current_time);
    }

    for (curr = sessions_.front(); curr; curr = next) {
        next = sessions_.nextof(*curr);

        core::nanoseconds_t sess_deadline = 0;

        if (!curr->refresh(current_time, &sess_deadline)) {
            // Session ended.
            remove_session_(curr);
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
            remove_session_(curr);
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

rtcp::ParticipantInfo ReceiverSessionGroup::participant_info() {
    rtcp::ParticipantInfo part_info;

    part_info.cname = identity_->cname();
    part_info.source_id = identity_->ssrc();
    part_info.report_back = true;

    return part_info;
}

void ReceiverSessionGroup::change_source_id() {
    identity_->change_ssrc();
}

size_t ReceiverSessionGroup::num_recv_streams() {
    // Gather report counts from all sessions.
    size_t n_reports = 0;

    for (core::SharedPtr<ReceiverSession> sess = sessions_.front(); sess;
         sess = sessions_.nextof(*sess)) {
        n_reports += sess->num_reports();
    }

    return n_reports;
}

void ReceiverSessionGroup::query_recv_streams(rtcp::RecvReport* reports,
                                              size_t n_reports,
                                              core::nanoseconds_t report_time) {
    roc_panic_if(!reports);

    // Gather reports from all sessions.
    for (core::SharedPtr<ReceiverSession> sess = sessions_.front(); sess;
         sess = sessions_.nextof(*sess)) {
        if (n_reports == 0) {
            break;
        }

        size_t n_sess_reports = sess->num_reports();
        if (n_sess_reports > n_reports) {
            n_sess_reports = n_reports;
        }

        sess->generate_reports(identity_->cname(), identity_->ssrc(), report_time,
                               reports, n_sess_reports);

        reports += n_sess_reports;
        n_reports -= n_sess_reports;
    }
}

status::StatusCode
ReceiverSessionGroup::notify_recv_stream(packet::stream_source_t send_source_id,
                                         const rtcp::SendReport& send_report) {
    // Remember session for given SSRC.
    core::SharedPtr<ReceiverSession> old_sess =
        session_router_.find_by_source(send_source_id);

    // Inform router that these CNAME and SSRC are related.
    // It is used to route related streams to the same session.
    const status::StatusCode code =
        session_router_.link_source(send_source_id, send_report.sender_cname);
    if (code != status::StatusOK) {
        roc_log(LogError, "session group: can't link source: status=%s",
                status::code_to_str(code));
        return code;
    }

    if (old_sess && !session_router_.has_session(old_sess)) {
        // If session existed before link_source(), but does not exist anymore, it
        // means that there are no more routes to that session.
        remove_session_(old_sess);
    }

    // If there is currently a session for given SSRC, let it process the report.
    core::SharedPtr<ReceiverSession> cur_sess =
        session_router_.find_by_source(send_source_id);
    if (cur_sess) {
        cur_sess->process_report(send_report);
    }

    return status::StatusOK;
}

void ReceiverSessionGroup::halt_recv_stream(packet::stream_source_t send_source_id) {
    // Remember session for given SSRC.
    core::SharedPtr<ReceiverSession> old_sess =
        session_router_.find_by_source(send_source_id);

    // Remove SSRC from router.
    session_router_.unlink_source(send_source_id);

    if (old_sess && !session_router_.has_session(old_sess)) {
        // If session existed before unlink_source(), but does not exist anymore, it
        // means that there are no more routes to that session.
        remove_session_(old_sess);
    }
}

status::StatusCode
ReceiverSessionGroup::route_transport_packet_(const packet::PacketPtr& packet) {
    // Find route by packet SSRC.
    core::SharedPtr<ReceiverSession> sess;

    if (packet->has_source_id()) {
        sess = session_router_.find_by_source(packet->source_id());
    }

    if (!sess && packet->udp()) {
        // If there is no route found, fallback to finding route by *source* address.
        //
        // We assume that packets sent from the same remote source address belong to the
        // same session.
        //
        // This does not conform to RFC 3550 (it mandates routing only by *destination*
        // address) and is not guaranteed to work, but it works in sample cases, assuming
        // that sender uses single port to send all packets (which is often the case) and
        // there are no retranslators involved (which is rarely the case).
        //
        // If we have functioning RTCP or RTSP, this fallback logic not used because we
        // will either find route based on SSRC, or will use separate destination
        // addresses (and hence separate session groups) for each sender.
        sess = session_router_.find_by_address(packet->udp()->src_addr);
    }

    if (sess) {
        // Session found, route packet to it.
        return sess->route_packet(packet);
    }

    // Session not found, auto-create session if possible.
    if (can_create_session_(packet)) {
        return create_session_(packet);
    }

    // TODO(gh-183): return status
    return status::StatusOK;
}

status::StatusCode
ReceiverSessionGroup::route_control_packet_(const packet::PacketPtr& packet,
                                            core::nanoseconds_t current_time) {
    if (!rtcp_communicator_) {
        roc_panic("session group: rtcp communicator is null");
    }

    // This will invoke IParticipant methods implemented by us,
    // in particular notify_recv_stream() and maybe halt_recv_stream().
    return rtcp_communicator_->process_packet(packet, current_time);
}

bool ReceiverSessionGroup::can_create_session_(const packet::PacketPtr& packet) {
    if (packet->has_flags(packet::Packet::FlagRepair)) {
        roc_log(LogDebug, "session group: ignoring repair packet for unknown session");
        return false;
    }

    return true;
}

status::StatusCode
ReceiverSessionGroup::create_session_(const packet::PacketPtr& packet) {
    if (!packet->rtp()) {
        roc_log(LogError,
                "session group: can't create session, unexpected non-rtp packet");
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    if (!packet->udp()) {
        roc_log(LogError,
                "session group: can't create session, unexpected non-udp packet");
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    const ReceiverSessionConfig sess_config = make_session_config_(packet);

    const packet::stream_source_t source_id = packet->source_id();

    const address::SocketAddr src_address = packet->udp()->src_addr;
    const address::SocketAddr dst_address = packet->udp()->dst_addr;

    roc_log(LogInfo, "session group: creating session: src_addr=%s dst_addr=%s",
            address::socket_addr_to_str(src_address).c_str(),
            address::socket_addr_to_str(dst_address).c_str());

    core::SharedPtr<ReceiverSession> sess = new (arena_) ReceiverSession(
        sess_config, receiver_config_.common, encoding_map_, packet_factory_,
        byte_buffer_factory_, sample_buffer_factory_, arena_);

    if (!sess || !sess->is_valid()) {
        roc_log(LogError, "session group: can't create session, initialization failed");
        // TODO(gh-183): return status
        return status::StatusOK;
    }

    status::StatusCode code = sess->route_packet(packet);
    if (code != status::StatusOK) {
        roc_log(
            LogError,
            "session group: can't create session, can't handle first packet: status=%s",
            status::code_to_str(code));
        // TODO(gh-183): handle and return status
        return status::StatusOK;
    }

    code = session_router_.add_session(sess, source_id, src_address);
    if (code != status::StatusOK) {
        roc_log(LogError,
                "session group: can't create session, can't create route: status=%s",
                status::code_to_str(code));
        // TODO(gh-183): handle and return status
        return status::StatusOK;
    }

    mixer_.add_input(sess->frame_reader());
    sessions_.push_back(*sess);

    state_tracker_.add_active_sessions(+1);

    return status::StatusOK;
}

void ReceiverSessionGroup::remove_session_(core::SharedPtr<ReceiverSession> sess) {
    roc_log(LogInfo, "session group: removing session");

    mixer_.remove_input(sess->frame_reader());
    sessions_.remove(*sess);

    session_router_.remove_session(sess);
    state_tracker_.add_active_sessions(-1);
}

void ReceiverSessionGroup::remove_all_sessions_() {
    roc_log(LogDebug, "session group: removing all sessions");

    while (!sessions_.is_empty()) {
        remove_session_(sessions_.back());
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
