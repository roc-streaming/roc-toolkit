/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_sink.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

SenderSink::SenderSink(const SenderConfig& config,
                       const rtp::EncodingMap& encoding_map,
                       packet::PacketFactory& packet_factory,
                       core::BufferFactory<uint8_t>& byte_buffer_factory,
                       core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                       core::IArena& arena)
    : config_(config)
    , encoding_map_(encoding_map)
    , packet_factory_(packet_factory)
    , byte_buffer_factory_(byte_buffer_factory)
    , sample_buffer_factory_(sample_buffer_factory)
    , arena_(arena)
    , frame_writer_(NULL)
    , valid_(false) {
    audio::IFrameWriter* awriter = &fanout_;

    if (config_.enable_profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingWriter(
            *awriter, arena, config_.input_sample_spec, config_.profiler_config));
        if (!profiler_ || !profiler_->is_valid()) {
            return;
        }
        awriter = profiler_.get();
    }

    if (!awriter) {
        return;
    }

    frame_writer_ = awriter;
    valid_ = true;
}

bool SenderSink::is_valid() const {
    return valid_;
}

SenderSlot* SenderSink::create_slot() {
    roc_panic_if(!is_valid());

    roc_log(LogInfo, "sender sink: adding slot");

    core::SharedPtr<SenderSlot> slot = new (arena_)
        SenderSlot(config_, state_tracker_, encoding_map_, fanout_, packet_factory_,
                   byte_buffer_factory_, sample_buffer_factory_, arena_);

    if (!slot || !slot->is_valid()) {
        roc_log(LogError, "sender sink: can't create slot");
        return NULL;
    }

    slots_.push_back(*slot);

    return slot.get();
}

void SenderSink::delete_slot(SenderSlot* slot) {
    roc_panic_if(!is_valid());

    roc_log(LogInfo, "sender sink: removing slot");

    slots_.remove(*slot);
}

size_t SenderSink::num_sessions() const {
    roc_panic_if(!is_valid());

    return state_tracker_.num_active_sessions();
}

core::nanoseconds_t SenderSink::refresh(core::nanoseconds_t current_time) {
    roc_panic_if(!is_valid());

    roc_panic_if_msg(current_time <= 0,
                     "sender sink: invalid timestamp:"
                     " expected positive value, got %lld",
                     (long long)current_time);

    core::nanoseconds_t next_deadline = 0;

    for (core::SharedPtr<SenderSlot> slot = slots_.front(); slot;
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

sndio::DeviceType SenderSink::type() const {
    return sndio::DeviceType_Sink;
}

sndio::DeviceState SenderSink::state() const {
    roc_panic_if(!is_valid());

    return state_tracker_.get_state();
}

void SenderSink::pause() {
    // no-op
}

bool SenderSink::resume() {
    return true;
}

bool SenderSink::restart() {
    return true;
}

audio::SampleSpec SenderSink::sample_spec() const {
    return config_.input_sample_spec;
}

core::nanoseconds_t SenderSink::latency() const {
    return 0;
}

bool SenderSink::has_latency() const {
    return false;
}

bool SenderSink::has_clock() const {
    return config_.enable_timing;
}

void SenderSink::write(audio::Frame& frame) {
    roc_panic_if(!is_valid());

    frame_writer_->write(frame);
}

} // namespace pipeline
} // namespace roc
