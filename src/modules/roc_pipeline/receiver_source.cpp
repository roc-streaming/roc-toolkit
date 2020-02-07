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

bool ReceiverSource::valid() const {
    return audio_reader_;
}

ReceiverSource::EndpointSetHandle ReceiverSource::add_endpoint_set() {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    roc_log(LogDebug, "receiver source: adding endpoint set");

    core::SharedPtr<ReceiverEndpointSet> endpoint_set = new (allocator_)
        ReceiverEndpointSet(config_, receiver_state_, *mixer_, format_map_, packet_pool_,
                            byte_buffer_pool_, sample_buffer_pool_, allocator_);

    if (!endpoint_set) {
        roc_log(LogError, "receiver source: can't allocate endpoint set");
        return 0;
    }

    endpoint_sets_.push_back(*endpoint_set);

    return (EndpointSetHandle)endpoint_set.get();
}

packet::IWriter* ReceiverSource::add_endpoint(EndpointSetHandle endpoint_set_handle,
                                              address::Interface iface,
                                              address::Protocol proto) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    ReceiverEndpointSet* endpoint_set = (ReceiverEndpointSet*)endpoint_set_handle;
    roc_panic_if_not(endpoint_set);

    return endpoint_set->add_endpoint(iface, proto);
}

void ReceiverSource::remove_endpoint(EndpointSetHandle endpoint_set_handle,
                                     address::Interface iface) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    ReceiverEndpointSet* endpoint_set = (ReceiverEndpointSet*)endpoint_set_handle;
    roc_panic_if_not(endpoint_set);

    endpoint_set->remove_endpoint(iface);
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
    roc_panic_if(!valid());

    if (receiver_state_.num_sessions() != 0) {
        // we have sessions and they're producing some sound
        return Playing;
    }

    if (receiver_state_.has_pending_packets()) {
        // we don't have sessions, but we have packets that may create sessions
        return Playing;
    }

    // no sessions and packets; we can sleep
    return Idle;
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

    roc_panic_if(!valid());

    if (config_.common.timing) {
        ticker_.wait(timestamp_);
    }

    for (core::SharedPtr<ReceiverEndpointSet> endpoint_set = endpoint_sets_.front();
         endpoint_set; endpoint_set = endpoint_sets_.nextof(*endpoint_set)) {
        endpoint_set->update(timestamp_);
    }

    if (!audio_reader_->read(frame)) {
        return false;
    }

    timestamp_ += frame.size() / num_channels_;

    return true;
}

} // namespace pipeline
} // namespace roc
