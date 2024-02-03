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

namespace roc {
namespace pipeline {

ReceiverSource::ReceiverSource(
    const ReceiverConfig& config,
    const rtp::EncodingMap& encoding_map,
    packet::PacketFactory& packet_factory,
    core::BufferFactory<uint8_t>& byte_buffer_factory,
    core::BufferFactory<audio::sample_t>& sample_buffer_factory,
    core::IArena& arena)
    : encoding_map_(encoding_map)
    , packet_factory_(packet_factory)
    , byte_buffer_factory_(byte_buffer_factory)
    , sample_buffer_factory_(sample_buffer_factory)
    , arena_(arena)
    , frame_reader_(NULL)
    , config_(config)
    , valid_(false) {
    config_.deduce_defaults();

    mixer_.reset(new (mixer_) audio::Mixer(sample_buffer_factory,
                                           config.common.output_sample_spec, true));
    if (!mixer_ || !mixer_->is_valid()) {
        return;
    }
    audio::IFrameReader* frm_reader = mixer_.get();

    if (config_.common.enable_profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingReader(
            *frm_reader, arena, config_.common.output_sample_spec,
            config_.common.profiler));
        if (!profiler_ || !profiler_->is_valid()) {
            return;
        }
        frm_reader = profiler_.get();
    }

    if (!frm_reader) {
        return;
    }

    frame_reader_ = frm_reader;
    valid_ = true;
}

bool ReceiverSource::is_valid() const {
    return valid_;
}

ReceiverSlot* ReceiverSource::create_slot() {
    roc_panic_if(!is_valid());

    roc_log(LogInfo, "receiver source: adding slot");

    core::SharedPtr<ReceiverSlot> slot = new (arena_)
        ReceiverSlot(config_, state_tracker_, *mixer_, encoding_map_, packet_factory_,
                     byte_buffer_factory_, sample_buffer_factory_, arena_);

    if (!slot || !slot->is_valid()) {
        roc_log(LogError, "receiver source: can't create slot");
        return NULL;
    }

    slots_.push_back(*slot);
    return slot.get();
}

void ReceiverSource::delete_slot(ReceiverSlot* slot) {
    roc_panic_if(!is_valid());

    roc_log(LogInfo, "receiver source: removing slot");

    slots_.remove(*slot);
}

size_t ReceiverSource::num_sessions() const {
    return state_tracker_.num_active_sessions();
}

core::nanoseconds_t ReceiverSource::refresh(core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    roc_panic_if_msg(current_time <= 0,
                     "receiver source: invalid timestamp:"
                     " expected positive value, got %lld",
                     (long long)current_time);

    core::nanoseconds_t next_deadline = 0;

    for (core::SharedPtr<ReceiverSlot> slot = slots_.front(); slot;
         slot = slots_.nextof(*slot)) {
        const core::nanoseconds_t slot_deadline = slot->refresh(current_time);

        if (slot_deadline != 0) {
            if (next_deadline == 0) {
                next_deadline = slot_deadline;
            } else {
                next_deadline = std::min(next_deadline, slot_deadline);
            }
        }
    }

    return next_deadline;
}

sndio::ISink* ReceiverSource::to_sink() {
    return NULL;
}

sndio::ISource* ReceiverSource::to_source() {
    return this;
}

sndio::DeviceType ReceiverSource::type() const {
    return sndio::DeviceType_Source;
}

sndio::DeviceState ReceiverSource::state() const {
    return state_tracker_.get_state();
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

bool ReceiverSource::has_latency() const {
    return false;
}

bool ReceiverSource::has_clock() const {
    return config_.common.enable_timing;
}

void ReceiverSource::reclock(core::nanoseconds_t playback_time) {
    roc_panic_if(!is_valid());

    roc_panic_if_msg(playback_time <= 0,
                     "receiver source: invalid timestamp:"
                     " expected positive value, got %lld",
                     (long long)playback_time);

    for (core::SharedPtr<ReceiverSlot> slot = slots_.front(); slot;
         slot = slots_.nextof(*slot)) {
        slot->reclock(playback_time);
    }
}

bool ReceiverSource::read(audio::Frame& frame) {
    roc_panic_if(!is_valid());

    return frame_reader_->read(frame);
}

} // namespace pipeline
} // namespace roc
