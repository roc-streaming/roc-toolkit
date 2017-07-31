/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace pipeline {

Receiver::Receiver(const ReceiverConfig& config,
                   const rtp::FormatMap& format_map,
                   packet::PacketPool& packet_pool,
                   core::BufferPool<uint8_t>& byte_buffer_pool,
                   core::BufferPool<audio::sample_t>& sample_buffer_pool,
                   core::IAllocator& allocator)
    : format_map_(format_map)
    , packet_pool_(packet_pool)
    , byte_buffer_pool_(byte_buffer_pool)
    , sample_buffer_pool_(sample_buffer_pool)
    , allocator_(allocator)
    , packet_queue_(0, false)
    , mixer_(sample_buffer_pool)
    , ticker_(config.sample_rate)
    , config_(config)
    , timestamp_(0)
    , num_channels_(packet::num_channels(config.channels)) {
}

bool Receiver::valid() {
    return true;
}

bool Receiver::add_port(const PortConfig& config) {
    core::SharedPtr<ReceiverPort> port =
        new (allocator_) ReceiverPort(config, format_map_, allocator_);

    if (!port || !port->valid()) {
        roc_log(LogError, "receiver: can't create port, initialization failed");
        return false;
    }

    ports_.push_back(*port);
    return true;
}

size_t Receiver::num_sessions() const {
    return sessions_.size();
}

void Receiver::write(const packet::PacketPtr& packet) {
    packet_queue_.write(packet);
}

IReceiver::Status Receiver::read(audio::Frame& frame) {
    if (config_.timing) {
        ticker_.wait(timestamp_);
    }

    fetch_packets_();

    const Status status = status_();

    mixer_.read(frame);
    timestamp_ += frame.samples.size() / num_channels_;

    update_sessions_();

    return status;
}

void Receiver::wait_active() {
    if (status_() == Active) {
        return;
    }

    packet_queue_.wait();
}

IReceiver::Status Receiver::status_() const {
    if (sessions_.size() != 0) {
        return Active;
    }

    if (packet_queue_.size() != 0) {
        return Active;
    }

    return Inactive;
}

void Receiver::fetch_packets_() {
    for (;;) {
        packet::PacketPtr packet = packet_queue_.read();
        if (!packet) {
            break;
        }

        if (!parse_packet_(packet)) {
            roc_log(LogDebug, "receiver: can't parse packet, dropping");
            continue;
        }

        if (!route_packet_(packet)) {
            roc_log(LogDebug, "receiver: can't route packet, dropping");
            continue;
        }
    }
}

bool Receiver::parse_packet_(const packet::PacketPtr& packet) {
    core::SharedPtr<ReceiverPort> port;

    for (port = ports_.front(); port; port = ports_.nextof(*port)) {
        if (port->handle(*packet)) {
            return true;
        }
    }

    return false;
}

bool Receiver::route_packet_(const packet::PacketPtr& packet) {
    core::SharedPtr<ReceiverSession> sess;

    for (sess = sessions_.front(); sess; sess = sessions_.nextof(*sess)) {
        if (sess->handle(packet)) {
            return true;
        }
    }

    return create_session_(packet);
}

bool Receiver::create_session_(const packet::PacketPtr& packet) {
    roc_log(LogInfo, "receiver: creating session");

    if (!packet->udp()) {
        roc_log(LogError, "receiver: can't create session, unexpected non-udp packet");
        return false;
    }
    const packet::Address src_address = packet->udp()->src_addr;

    core::SharedPtr<ReceiverSession> sess = new (allocator_)
        ReceiverSession(config_.default_session, src_address, format_map_, packet_pool_,
                        byte_buffer_pool_, sample_buffer_pool_, allocator_);

    if (!sess || !sess->valid()) {
        roc_log(LogError, "receiver: can't create session, initialization failed");
        return false;
    }

    if (!sess->handle(packet)) {
        roc_log(LogError, "receiver: can't create session, can't handle first packet");
        return false;
    }

    mixer_.add(sess->reader());
    sessions_.push_back(*sess);

    return true;
}

void Receiver::remove_session_(ReceiverSession& sess) {
    roc_log(LogInfo, "receiver: removing session");

    mixer_.remove(sess.reader());
    sessions_.remove(sess);
}

void Receiver::update_sessions_() {
    core::SharedPtr<ReceiverSession> curr, next;

    for (curr = sessions_.front(); curr; curr = next) {
        next = sessions_.nextof(*curr);

        if (!curr->update(timestamp_)) {
            remove_session_(*curr);
        }
    }
}

} // namespace pipeline
} // namespace roc
