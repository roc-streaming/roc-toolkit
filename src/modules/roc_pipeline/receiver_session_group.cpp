/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_session_group.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"

namespace roc {
namespace pipeline {

ReceiverSessionGroup::ReceiverSessionGroup(
    const ReceiverConfig& receiver_config,
    ReceiverState& receiver_state,
    audio::Mixer& mixer,
    const fec::CodecMap& codec_map,
    const rtp::FormatMap& format_map,
    packet::PacketPool& packet_pool,
    core::BufferPool<uint8_t>& byte_buffer_pool,
    core::BufferPool<audio::sample_t>& sample_buffer_pool,
    core::IAllocator& allocator)
    : allocator_(allocator)
    , packet_pool_(packet_pool)
    , byte_buffer_pool_(byte_buffer_pool)
    , sample_buffer_pool_(sample_buffer_pool)
    , codec_map_(codec_map)
    , format_map_(format_map)
    , mixer_(mixer)
    , receiver_state_(receiver_state)
    , receiver_config_(receiver_config) {
}

void ReceiverSessionGroup::route_packet(const packet::PacketPtr& packet) {
    core::SharedPtr<ReceiverSession> sess;

    for (sess = sessions_.front(); sess; sess = sessions_.nextof(*sess)) {
        if (sess->handle(packet)) {
            return;
        }
    }

    if (can_create_session_(packet)) {
        create_session_(packet);
    }
}

void ReceiverSessionGroup::update_sessions(packet::timestamp_t timestamp) {
    core::SharedPtr<ReceiverSession> curr, next;

    for (curr = sessions_.front(); curr; curr = next) {
        next = sessions_.nextof(*curr);

        if (!curr->update(timestamp)) {
            remove_session_(*curr);
        }
    }
}

size_t ReceiverSessionGroup::num_sessions() const {
    return sessions_.size();
}

bool ReceiverSessionGroup::can_create_session_(const packet::PacketPtr& packet) {
    if (packet->flags() & packet::Packet::FlagRepair) {
        roc_log(LogDebug, "session group: ignoring repair packet for unknown session");
        return false;
    }

    return true;
}

void ReceiverSessionGroup::create_session_(const packet::PacketPtr& packet) {
    if (!packet->udp()) {
        roc_log(LogError,
                "session group: can't create session, unexpected non-udp packet");
        return;
    }

    if (!packet->rtp()) {
        roc_log(LogError,
                "session group: can't create session, unexpected non-rtp packet");
        return;
    }

    const ReceiverSessionConfig sess_config = make_session_config_(packet);

    const address::SocketAddr src_address = packet->udp()->src_addr;
    const address::SocketAddr dst_address = packet->udp()->dst_addr;

    roc_log(LogInfo, "session group: creating session: src_addr=%s dst_addr=%s",
            address::socket_addr_to_str(src_address).c_str(),
            address::socket_addr_to_str(dst_address).c_str());

    core::SharedPtr<ReceiverSession> sess = new (allocator_) ReceiverSession(
        sess_config, receiver_config_.common, src_address, codec_map_, format_map_,
        packet_pool_, byte_buffer_pool_, sample_buffer_pool_, allocator_);

    if (!sess || !sess->valid()) {
        roc_log(LogError, "session group: can't create session, initialization failed");
        return;
    }

    if (!sess->handle(packet)) {
        roc_log(LogError,
                "session group: can't create session, can't handle first packet");
        return;
    }

    mixer_.add_input(sess->reader());
    sessions_.push_back(*sess);

    receiver_state_.add_sessions(+1);
}

void ReceiverSessionGroup::remove_session_(ReceiverSession& sess) {
    roc_log(LogInfo, "session group: removing session");

    mixer_.remove_input(sess.reader());
    sessions_.remove(sess);

    receiver_state_.add_sessions(-1);
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
