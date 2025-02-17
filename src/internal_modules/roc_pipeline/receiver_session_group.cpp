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

ReceiverSessionGroup::ReceiverSessionGroup(const ReceiverSourceConfig& source_config,
                                           const ReceiverSlotConfig& slot_config,
                                           StateTracker& state_tracker,
                                           audio::Mixer& mixer,
                                           audio::ProcessorMap& processor_map,
                                           rtp::EncodingMap& encoding_map,
                                           packet::PacketFactory& packet_factory,
                                           audio::FrameFactory& frame_factory,
                                           core::IArena& arena,
                                           dbgio::CsvDumper* dumper)
    : source_config_(source_config)
    , slot_config_(slot_config)
    , state_tracker_(state_tracker)
    , mixer_(mixer)
    , processor_map_(processor_map)
    , encoding_map_(encoding_map)
    , arena_(arena)
    , packet_factory_(packet_factory)
    , frame_factory_(frame_factory)
    , session_router_(arena)
    , dumper_(dumper)
    , init_status_(status::NoStatus) {
    identity_.reset(new (identity_) rtp::Identity());
    if ((init_status_ = identity_->init_status()) != status::StatusOK) {
        return;
    }

    init_status_ = status::StatusOK;
}

ReceiverSessionGroup::~ReceiverSessionGroup() {
    remove_all_sessions_();
}

status::StatusCode ReceiverSessionGroup::init_status() const {
    return init_status_;
}

status::StatusCode
ReceiverSessionGroup::create_control_pipeline(ReceiverEndpoint* control_endpoint) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!control_endpoint);
    roc_panic_if(!control_endpoint->outbound_composer()
                 || !control_endpoint->outbound_writer());
    roc_panic_if(rtcp_communicator_);

    // We will use this address when returning information for
    // rtcp::Communicator in participant_info().
    rtcp_inbound_addr_ = control_endpoint->inbound_address();

    // We pass this as implementation of rtcp::IParticipant.
    // rtcp::Communicator will call our methods right now (in constructor)
    // and later when we call generate_packets() or process_packets().
    rtcp_communicator_.reset(new (rtcp_communicator_) rtcp::Communicator(
        source_config_.common.rtcp, *this, *control_endpoint->outbound_writer(),
        *control_endpoint->outbound_composer(), packet_factory_, arena_));

    const status::StatusCode code = rtcp_communicator_->init_status();
    if (code != status::StatusOK) {
        rtcp_communicator_.reset();
        rtcp_inbound_addr_.clear();
        return code;
    }

    return status::StatusOK;
}

status::StatusCode
ReceiverSessionGroup::refresh_sessions(core::nanoseconds_t current_time,
                                       core::nanoseconds_t& next_deadline) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (rtcp_communicator_) {
        // This will invoke IParticipant methods implemented by us,
        // in particular query_recv_streams().
        const status::StatusCode code =
            rtcp_communicator_->generate_reports(current_time);

        if (code != status::StatusOK) {
            return code;
        }

        next_deadline = rtcp_communicator_->generation_deadline(current_time);
    }

    core::SharedPtr<ReceiverSession> curr_sess, next_sess;

    for (curr_sess = sessions_.front(); curr_sess; curr_sess = next_sess) {
        next_sess = sessions_.nextof(*curr_sess);

        core::nanoseconds_t sess_deadline = 0;
        const status::StatusCode code = curr_sess->refresh(current_time, sess_deadline);

        // These errors break only session, but not the whole receiver.
        if (code == status::StatusFinish || code == status::StatusAbort) {
            remove_session_(curr_sess, code);
            continue;
        }

        if (code != status::StatusOK) {
            return code;
        }

        if (sess_deadline != 0) {
            next_deadline = (next_deadline == 0 ? sess_deadline
                                                : std::min(next_deadline, sess_deadline));
        }
    }

    return status::StatusOK;
}

void ReceiverSessionGroup::reclock_sessions(core::nanoseconds_t playback_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    core::SharedPtr<ReceiverSession> curr_sess, next_sess;

    for (curr_sess = sessions_.front(); curr_sess; curr_sess = next_sess) {
        next_sess = sessions_.nextof(*curr_sess);

        curr_sess->reclock(playback_time);
    }
}

status::StatusCode ReceiverSessionGroup::route_packet(const packet::PacketPtr& packet,
                                                      core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (packet->has_flags(packet::Packet::FlagControl)) {
        return route_control_packet_(packet, current_time);
    }

    return route_transport_packet_(packet);
}

size_t ReceiverSessionGroup::num_sessions() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return sessions_.size();
}

void ReceiverSessionGroup::get_slot_metrics(ReceiverSlotMetrics& slot_metrics) const {
    roc_panic_if(init_status_ != status::StatusOK);

    slot_metrics.source_id = identity_->ssrc();
    slot_metrics.num_participants = sessions_.size();
}

void ReceiverSessionGroup::get_participant_metrics(
    ReceiverParticipantMetrics* party_metrics, size_t* party_count) const {
    roc_panic_if(init_status_ != status::StatusOK);

    if (party_metrics && party_count) {
        *party_count = std::min(*party_count, sessions_.size());

        size_t n_part = 0;
        for (core::SharedPtr<ReceiverSession> sess = sessions_.front(); sess;
             sess = sessions_.nextof(*sess)) {
            if (n_part == *party_count) {
                break;
            }
            party_metrics[n_part++] = sess->get_metrics();
        }
    } else if (party_count) {
        *party_count = 0;
    }
}

rtcp::ParticipantInfo ReceiverSessionGroup::participant_info() {
    rtcp::ParticipantInfo part_info;

    part_info.cname = identity_->cname();
    part_info.source_id = identity_->ssrc();

    if (rtcp_inbound_addr_.multicast()) {
        part_info.report_mode = rtcp::Report_ToAddress;
        part_info.report_address = rtcp_inbound_addr_;
    } else {
        part_info.report_mode = rtcp::Report_Back;
    }

    return part_info;
}

void ReceiverSessionGroup::change_source_id() {
    const status::StatusCode code = identity_->change_ssrc();

    if (code != status::StatusOK) {
        roc_panic("session group: can't change SSRC: status=%s",
                  status::code_to_str(code));
    }
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
        remove_session_(old_sess, status::NoStatus);
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
        remove_session_(old_sess, status::NoStatus);
    }
}

status::StatusCode
ReceiverSessionGroup::route_transport_packet_(const packet::PacketPtr& packet) {
    core::SharedPtr<ReceiverSession> sess;

    if (slot_config_.enable_routing) {
        // Find route by packet SSRC.
        if (packet->has_source_id()) {
            sess = session_router_.find_by_source(packet->source_id());
        }

        if (!sess && packet->udp()) {
            // If there is no route found, fallback to finding route by *source* address.
            //
            // We assume that packets sent from the same remote source address belong to
            // the same session.
            //
            // This does not conform to RFC 3550 (it mandates routing only by
            // *destination* address) and is not guaranteed to work, but it works in
            // simple cases, assuming that sender uses single port to send all packets
            // (which is often the case) and there are no retranslators involved (which is
            // rarely the case).
            //
            // If we have functioning RTCP or RTSP, this fallback logic isn't used because
            // we'll either find route based on SSRC, or will use separate destination
            // addresses (and hence separate session groups) for each sender.
            sess = session_router_.find_by_address(packet->udp()->src_addr);
        }
    } else {
        // If routing is disabled, we can only have zero or one session.
        roc_panic_if_not(sessions_.size() == 0 || sessions_.size() == 1);

        if (!sessions_.is_empty()) {
            sess = sessions_.front();
        }
    }

    if (sess) {
        // Session found, route packet to it.
        return sess->route_packet(packet);
    }

    // Session not found, auto-create session if possible.
    if (can_create_session_(packet)) {
        return create_session_(packet);
    }

    return status::StatusNoRoute;
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
    if (!packet->has_flags(packet::Packet::FlagRTP)) {
        roc_log(LogError,
                "session group: can't create session, unexpected non-rtp packet");
        return status::StatusNoRoute;
    }

    if (!packet->has_flags(packet::Packet::FlagUDP)) {
        roc_log(LogError,
                "session group: can't create session, unexpected non-udp packet");
        return status::StatusNoRoute;
    }

    const ReceiverSessionConfig sess_config = make_session_config_(packet);

    const packet::stream_source_t source_id = packet->source_id();

    const address::SocketAddr& src_address = packet->udp()->src_addr;
    const address::SocketAddr& dst_address = packet->udp()->dst_addr;

    roc_log(LogInfo, "session group: creating session: src_addr=%s dst_addr=%s",
            address::socket_addr_to_str(src_address).c_str(),
            address::socket_addr_to_str(dst_address).c_str());

    core::SharedPtr<ReceiverSession> sess = new (arena_)
        ReceiverSession(sess_config, source_config_.common, processor_map_, encoding_map_,
                        packet_factory_, frame_factory_, arena_, dumper_);

    if (!sess) {
        roc_log(LogError, "session group: can't create session, allocation failed");
        return status::StatusNoMem;
    }

    if (sess->init_status() != status::StatusOK) {
        roc_log(LogError,
                "session group: can't create session, initialization failed: status=%s",
                status::code_to_str(sess->init_status()));
        return sess->init_status();
    }

    status::StatusCode code = sess->route_packet(packet);
    if (code != status::StatusOK) {
        roc_log(
            LogError,
            "session group: can't create session, can't handle first packet: status=%s",
            status::code_to_str(code));
        return code;
    }

    code = session_router_.add_session(sess, source_id, src_address);
    if (code != status::StatusOK) {
        roc_log(LogError,
                "session group: can't create session, can't create route: status=%s",
                status::code_to_str(code));
        return code;
    }

    code = mixer_.add_input(sess->frame_reader());
    if (code != status::StatusOK) {
        roc_log(LogError,
                "session group: can't create session, can't add input: status=%s",
                status::code_to_str(code));
        session_router_.remove_session(sess);
        return code;
    }

    sessions_.push_back(*sess);
    state_tracker_.register_session();

    return status::StatusOK;
}

void ReceiverSessionGroup::remove_session_(core::SharedPtr<ReceiverSession> sess,
                                           status::StatusCode code) {
    if (code != status::NoStatus) {
        roc_log(LogInfo, "session group: removing session: status=%s",
                status::code_to_str(code));
    } else {
        roc_log(LogInfo, "session group: removing session");
    }

    mixer_.remove_input(sess->frame_reader());
    sessions_.remove(*sess);

    session_router_.remove_session(sess);
    state_tracker_.unregister_session();
}

void ReceiverSessionGroup::remove_all_sessions_() {
    roc_log(LogDebug, "session group: removing all sessions");

    while (!sessions_.is_empty()) {
        remove_session_(sessions_.back(), status::NoStatus);
    }
}

ReceiverSessionConfig
ReceiverSessionGroup::make_session_config_(const packet::PacketPtr& packet) const {
    ReceiverSessionConfig config = source_config_.session_defaults;

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
