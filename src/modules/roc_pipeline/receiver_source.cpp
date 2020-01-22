/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_source.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace pipeline {

ReceiverSource::ReceiverSource(const ReceiverConfig& config,
                               const fec::CodecMap& codec_map,
                               const rtp::FormatMap& format_map,
                               packet::PacketPool& packet_pool,
                               core::BufferPool<uint8_t>& byte_buffer_pool,
                               core::BufferPool<audio::sample_t>& sample_buffer_pool,
                               core::IAllocator& allocator)
    : codec_map_(codec_map)
    , format_map_(format_map)
    , packet_pool_(packet_pool)
    , byte_buffer_pool_(byte_buffer_pool)
    , sample_buffer_pool_(sample_buffer_pool)
    , allocator_(allocator)
    , ticker_(config.common.output_sample_rate)
    , audio_reader_(NULL)
    , config_(config)
    , timestamp_(0)
    , num_channels_(packet::num_channels(config.common.output_channels)) {
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

bool ReceiverSource::valid() {
    return audio_reader_;
}

ReceiverSource::PortGroupID ReceiverSource::add_port_group() {
    core::Mutex::Lock lock(mutex_);

    roc_log(LogInfo, "receiver source: adding port group");

    core::SharedPtr<ReceiverPortGroup> port_group = new (allocator_) ReceiverPortGroup(
        config_, receiver_state_, *mixer_, codec_map_, format_map_, packet_pool_,
        byte_buffer_pool_, sample_buffer_pool_, allocator_);

    if (!port_group) {
        roc_log(LogError, "receiver source: can't allocate port group");
        return 0;
    }

    port_groups_.push_back(*port_group);

    return (PortGroupID)port_group.get();
}

packet::IWriter* ReceiverSource::add_port(PortGroupID port_group,
                                          address::EndpointProtocol port_proto) {
    core::Mutex::Lock lock(mutex_);

    roc_log(LogInfo, "receiver source: adding port");

    ReceiverPortGroup* port_group_ptr = (ReceiverPortGroup*)port_group;
    roc_panic_if_not(port_group_ptr);

    return port_group_ptr->add_port(port_proto);
}

size_t ReceiverSource::num_sessions() const {
    return receiver_state_.num_sessions();
}

size_t ReceiverSource::sample_rate() const {
    return config_.common.output_sample_rate;
}

size_t ReceiverSource::num_channels() const {
    return num_channels_;
}

bool ReceiverSource::has_clock() const {
    return config_.common.timing;
}

sndio::ISource::State ReceiverSource::state() const {
    if (receiver_state_.num_sessions() != 0) {
        // we have sessions and they're producing some sound
        return Active;
    }

    if (receiver_state_.has_pending_packets()) {
        // we don't have sessions, but we have packets that may create sessions
        return Active;
    }

    // no sessions and packets; we can sleep
    return Inactive;
}

void ReceiverSource::pause() {
    // no-op
}

bool ReceiverSource::resume() {
    return true;
}

bool ReceiverSource::restart() {
    return true;
}

bool ReceiverSource::read(audio::Frame& frame) {
    core::Mutex::Lock lock(mutex_);

    if (config_.common.timing) {
        ticker_.wait(timestamp_);
    }

    for (core::SharedPtr<ReceiverPortGroup> port_group = port_groups_.front(); port_group;
         port_group = port_groups_.nextof(*port_group)) {
        port_group->update(timestamp_);
    }

    if (!audio_reader_->read(frame)) {
        return false;
    }

    timestamp_ += frame.size() / num_channels_;

    return true;
}

} // namespace pipeline
} // namespace roc
