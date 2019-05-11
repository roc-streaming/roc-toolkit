/*
 * Copyright (c) 2017 Roc authors
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
    , ticker_(config.common.output_sample_rate)
    , audio_reader_(NULL)
    , config_(config)
    , timestamp_(0)
    , num_channels_(packet::num_channels(config.common.output_channels))
    , active_cond_(control_mutex_) {
    mixer_.reset(new (allocator_)
                     audio::Mixer(sample_buffer_pool, config.common.internal_frame_size),
                 allocator_);
    if (!mixer_ || !mixer_->valid()) {
        return;
    }
    audio::IReader* areader = mixer_.get();

    if (config.common.poisoning) {
        poisoner_.reset(new (allocator_) audio::PoisonReader(*areader), allocator_);
        if (!poisoner_) {
            return;
        }
        areader = poisoner_.get();
    }

    audio_reader_ = areader;
}

bool Receiver::valid() {
    return audio_reader_;
}

bool Receiver::add_port(const PortConfig& config) {
    core::Mutex::Lock lock(control_mutex_);

    core::SharedPtr<ReceiverPort> port =
        new (allocator_) ReceiverPort(config, format_map_, allocator_);

    if (!port || !port->valid()) {
        roc_log(LogError, "receiver: can't create port, initialization failed");
        return false;
    }

    ports_.push_back(*port);
    return true;
}

void Receiver::iterate_ports(void (*fn)(void*, const PortConfig&), void* arg) const {
    core::Mutex::Lock lock(control_mutex_);

    core::SharedPtr<ReceiverPort> port;

    for (port = ports_.front(); port; port = ports_.nextof(*port)) {
        fn(arg, port->config());
    }
}

size_t Receiver::num_sessions() const {
    core::Mutex::Lock lock(control_mutex_);

    return sessions_.size();
}

void Receiver::write(const packet::PacketPtr& packet) {
    core::Mutex::Lock lock(control_mutex_);

    const State old_state = state_();

    packets_.push_back(*packet);

    if (old_state != Active) {
        active_cond_.broadcast();
    }
}

bool Receiver::read(audio::Frame& frame) {
    core::Mutex::Lock lock(pipeline_mutex_);

    if (config_.common.timing) {
        ticker_.wait(timestamp_);
    }

    prepare_();

    audio_reader_->read(frame);
    timestamp_ += frame.size() / num_channels_;

    return true;
}

sndio::ISource::State Receiver::state() const {
    core::Mutex::Lock lock(control_mutex_);

    return state_();
}

void Receiver::wait_active() const {
    core::Mutex::Lock lock(control_mutex_);

    while (state_() != Active) {
        active_cond_.wait();
    }
}

void Receiver::prepare_() {
    core::Mutex::Lock lock(control_mutex_);

    const State old_state = state_();

    fetch_packets_();
    update_sessions_();

    if (old_state != Active && state_() == Active) {
        active_cond_.broadcast();
    }
}

sndio::ISource::State Receiver::state_() const {
    if (sessions_.size() != 0) {
        return Active;
    }

    if (packets_.size() != 0) {
        return Active;
    }

    return Inactive;
}

void Receiver::fetch_packets_() {
    for (;;) {
        packet::PacketPtr packet = packets_.front();
        if (!packet) {
            break;
        }

        packets_.remove(*packet);

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

    if (!packet->rtp()) {
        roc_log(LogError, "receiver: can't create session, unexpected non-rtp packet");
        return false;
    }

    if (packet->flags() & packet::Packet::FlagRepair) {
        roc_log(LogDebug, "receiver: dropping repair packet for non-existent session");
        return false;
    }

    const packet::Address src_address = packet->udp()->src_addr;

    core::SharedPtr<ReceiverSession> sess = new (allocator_) ReceiverSession(
        config_.default_session, config_.common, packet->rtp()->payload_type, src_address,
        format_map_, packet_pool_, byte_buffer_pool_, sample_buffer_pool_, allocator_);

    if (!sess || !sess->valid()) {
        roc_log(LogError, "receiver: can't create session, initialization failed");
        return false;
    }

    if (!sess->handle(packet)) {
        roc_log(LogError, "receiver: can't create session, can't handle first packet");
        return false;
    }

    mixer_->add(sess->reader());
    sessions_.push_back(*sess);

    return true;
}

void Receiver::remove_session_(ReceiverSession& sess) {
    roc_log(LogInfo, "receiver: removing session");

    mixer_->remove(sess.reader());
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
