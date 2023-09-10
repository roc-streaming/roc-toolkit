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
#include "roc_core/stddefs.h"
#include "roc_sndio/device_state.h"

namespace roc {
namespace pipeline {

SenderSink::SenderSink(const SenderConfig& config,
                       const rtp::FormatMap& format_map,
                       packet::PacketFactory& packet_factory,
                       core::BufferFactory<uint8_t>& byte_buffer_factory,
                       core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                       core::IArena& arena)
    : config_(config)
    , format_map_(format_map)
    , packet_factory_(packet_factory)
    , byte_buffer_factory_(byte_buffer_factory)
    , sample_buffer_factory_(sample_buffer_factory)
    , arena_(arena)
    , audio_writer_(NULL)
    , update_deadline_valid_(false)
    , update_deadline_(0) {
    audio::IFrameWriter* awriter = &fanout_;

    if (config_.enable_profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingWriter(
            *awriter, arena, config_.input_sample_spec, config_.profiler_config));
        if (!profiler_ || !profiler_->is_valid()) {
            return;
        }
        awriter = profiler_.get();
    }

    audio_writer_ = awriter;
}

bool SenderSink::is_valid() const {
    return audio_writer_;
}

SenderSlot* SenderSink::create_slot() {
    roc_panic_if(!is_valid());

    roc_log(LogInfo, "sender sink: adding slot");

    core::SharedPtr<SenderSlot> slot =
        new (arena_) SenderSlot(config_, format_map_, fanout_, packet_factory_,
                                byte_buffer_factory_, sample_buffer_factory_, arena_);

    if (!slot) {
        roc_log(LogError, "sender sink: can't allocate slot");
        return NULL;
    }

    slots_.push_back(*slot);

    invalidate_update_deadline_();

    return slot.get();
}

void SenderSink::delete_slot(SenderSlot* slot) {
    roc_panic_if(!is_valid());

    roc_log(LogInfo, "sender sink: removing slot");

    slots_.remove(*slot);

    invalidate_update_deadline_();
}

core::nanoseconds_t SenderSink::get_update_deadline(core::nanoseconds_t current_time) {
    if (!update_deadline_valid_) {
        compute_update_deadline_(current_time);
    }

    return update_deadline_;
}

void SenderSink::update(core::nanoseconds_t current_time) {
    core::SharedPtr<SenderSlot> slot;

    for (slot = slots_.front(); slot; slot = slots_.nextof(*slot)) {
        slot->update(current_time);
    }

    invalidate_update_deadline_();
}

sndio::DeviceType SenderSink::type() const {
    return sndio::DeviceType_Sink;
}

sndio::DeviceState SenderSink::state() const {
    return sndio::DeviceState_Active;
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

    audio_writer_->write(frame);
}

void SenderSink::compute_update_deadline_(core::nanoseconds_t current_time) {
    core::SharedPtr<SenderSlot> slot;

    update_deadline_ = 0;

    for (slot = slots_.front(); slot; slot = slots_.nextof(*slot)) {
        const core::nanoseconds_t deadline = slot->get_update_deadline(current_time);
        if (deadline == 0) {
            continue;
        }

        if (update_deadline_ == 0) {
            update_deadline_ = deadline;
        } else {
            update_deadline_ = std::min(update_deadline_, deadline);
        }
    }

    update_deadline_valid_ = true;
}

void SenderSink::invalidate_update_deadline_() {
    update_deadline_ = 0;
    update_deadline_valid_ = false;
}

} // namespace pipeline
} // namespace roc
