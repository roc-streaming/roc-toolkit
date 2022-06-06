/*
 * Copyright (c) 2017 Roc Streaming authors
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

ReceiverSource::ReceiverSource(
    const ReceiverConfig& config,
    const rtp::FormatMap& format_map,
    packet::PacketFactory& packet_factory,
    core::BufferFactory<uint8_t>& byte_buffer_factory,
    core::BufferFactory<audio::sample_t>& sample_buffer_factory,
    core::IAllocator& allocator)
    : format_map_(format_map)
    , packet_factory_(packet_factory)
    , byte_buffer_factory_(byte_buffer_factory)
    , sample_buffer_factory_(sample_buffer_factory)
    , allocator_(allocator)
    , audio_reader_(NULL)
    , config_(config)
    , timestamp_(0) {
    mixer_.reset(new (mixer_) audio::Mixer(sample_buffer_factory,
                                           config.common.internal_frame_length,
                                           config.common.output_sample_spec));
    if (!mixer_ || !mixer_->valid()) {
        return;
    }
    audio::IReader* areader = mixer_.get();

    if (config.common.poisoning) {
        poisoner_.reset(new (poisoner_) audio::PoisonReader(*areader));
        if (!poisoner_) {
            return;
        }
        areader = poisoner_.get();
    }

    if (config.common.profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingReader(
            *areader, allocator, config.common.output_sample_spec,
            config.common.profiler_config));
        if (!profiler_ || !profiler_->valid()) {
            return;
        }
        areader = profiler_.get();
    }

    audio_reader_ = areader;
}

bool ReceiverSource::valid() const {
    return audio_reader_;
}

ReceiverEndpointSet* ReceiverSource::create_endpoint_set() {
    core::SharedPtr<ReceiverEndpointSet> endpoint_set = new (allocator_)
        ReceiverEndpointSet(config_, receiver_state_, *mixer_, format_map_,
                            packet_factory_, byte_buffer_factory_, sample_buffer_factory_,
                            allocator_);
    if (!endpoint_set) {
        return NULL;
    }

    endpoint_sets_.push_back(*endpoint_set);
    return endpoint_set.get();
}

size_t ReceiverSource::num_sessions() const {
    return receiver_state_.num_sessions();
}

size_t ReceiverSource::sample_rate() const {
    return config_.common.output_sample_spec.sample_rate();
}

size_t ReceiverSource::num_channels() const {
    return config_.common.output_sample_spec.num_channels();
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

    // no sessions and packets; we can sleep until there are some
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
    roc_panic_if(!valid());

    for (core::SharedPtr<ReceiverEndpointSet> endpoint_set = endpoint_sets_.front();
         endpoint_set; endpoint_set = endpoint_sets_.nextof(*endpoint_set)) {
        endpoint_set->update(timestamp_);
    }

    if (!audio_reader_->read(frame)) {
        return false;
    }

    timestamp_ += frame.size() / config_.common.output_sample_spec.num_channels();

    return true;
}

} // namespace pipeline
} // namespace roc
