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
#include "roc_sndio/device_type.h"

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
    audio::IFrameReader* areader = mixer_.get();

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

ReceiverSlot* ReceiverSource::create_slot() {
    core::SharedPtr<ReceiverSlot> slot = new (allocator_)
        ReceiverSlot(config_, state_, *mixer_, format_map_, packet_factory_,
                     byte_buffer_factory_, sample_buffer_factory_, allocator_);
    if (!slot) {
        return NULL;
    }

    slots_.push_back(*slot);
    return slot.get();
}

size_t ReceiverSource::num_sessions() const {
    return state_.num_sessions();
}

sndio::DeviceType ReceiverSource::type() const {
    return sndio::DeviceType_Source;
}

sndio::DeviceState ReceiverSource::state() const {
    roc_panic_if(!valid());

    if (state_.num_sessions() != 0) {
        // we have sessions and they're producing some sound
        return sndio::DeviceState_Active;
    }

    if (state_.has_pending_packets()) {
        // we don't have sessions, but we have packets that may create sessions
        return sndio::DeviceState_Active;
    }

    // no sessions and packets; we can sleep until there are some
    return sndio::DeviceState_Idle;
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

audio::SampleSpec ReceiverSource::sample_spec() const {
    return config_.common.output_sample_spec;
}

core::nanoseconds_t ReceiverSource::latency() const {
    return 0;
}

bool ReceiverSource::has_clock() const {
    return config_.common.timing;
}

void ReceiverSource::reclock(packet::ntp_timestamp_t timestamp) {
    roc_panic_if(!valid());

    for (core::SharedPtr<ReceiverSlot> slot = slots_.front(); slot;
         slot = slots_.nextof(*slot)) {
        slot->reclock(timestamp);
    }
}

bool ReceiverSource::read(audio::Frame& frame) {
    roc_panic_if(!valid());

    for (core::SharedPtr<ReceiverSlot> slot = slots_.front(); slot;
         slot = slots_.nextof(*slot)) {
        slot->advance(timestamp_);
    }

    if (!audio_reader_->read(frame)) {
        return false;
    }

    timestamp_ += packet::timestamp_t(frame.num_samples()
                                      / config_.common.output_sample_spec.num_channels());

    return true;
}

} // namespace pipeline
} // namespace roc
