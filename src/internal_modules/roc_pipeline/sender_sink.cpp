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
#include "roc_status/code_to_str.h"

namespace roc {
namespace pipeline {

SenderSink::SenderSink(const SenderSinkConfig& sink_config,
                       const rtp::EncodingMap& encoding_map,
                       core::IPool& packet_pool,
                       core::IPool& packet_buffer_pool,
                       core::IPool& frame_buffer_pool,
                       core::IArena& arena)
    : sink_config_(sink_config)
    , encoding_map_(encoding_map)
    , packet_factory_(packet_pool, packet_buffer_pool)
    , frame_factory_(frame_buffer_pool)
    , arena_(arena)
    , frame_writer_(NULL)
    , init_status_(status::NoStatus) {
    sink_config_.deduce_defaults();

    audio::IFrameWriter* frm_writer = &fanout_;

    if (!sink_config_.input_sample_spec.is_raw()) {
        const audio::SampleSpec out_spec(sink_config_.input_sample_spec.sample_rate(),
                                         audio::Sample_RawFormat,
                                         sink_config_.input_sample_spec.channel_set());

        pcm_mapper_.reset(new (pcm_mapper_) audio::PcmMapperWriter(
            *frm_writer, frame_factory_, sink_config_.input_sample_spec, out_spec));
        if ((init_status_ = pcm_mapper_->init_status()) != status::StatusOK) {
            return;
        }
        frm_writer = pcm_mapper_.get();
    }

    if (sink_config_.enable_profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingWriter(
            *frm_writer, arena, sink_config_.input_sample_spec, sink_config_.profiler));
        if ((init_status_ = profiler_->init_status()) != status::StatusOK) {
            return;
        }
        frm_writer = profiler_.get();
    }

    frame_writer_ = frm_writer;
    init_status_ = status::StatusOK;
}

status::StatusCode SenderSink::init_status() const {
    return init_status_;
}

SenderSlot* SenderSink::create_slot(const SenderSlotConfig& slot_config) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_log(LogInfo, "sender sink: adding slot");

    core::SharedPtr<SenderSlot> slot =
        new (arena_) SenderSlot(sink_config_, slot_config, state_tracker_, encoding_map_,
                                fanout_, packet_factory_, frame_factory_, arena_);

    if (!slot || slot->init_status() != status::StatusOK) {
        roc_log(LogError, "sender sink: can't create slot: status=%s",
                status::code_to_str(slot->init_status()));
        // TODO(gh-183): forward status (control ops)
        return NULL;
    }

    slots_.push_back(*slot);

    return slot.get();
}

void SenderSink::delete_slot(SenderSlot* slot) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_log(LogInfo, "sender sink: removing slot");

    slots_.remove(*slot);
}

size_t SenderSink::num_sessions() const {
    roc_panic_if(init_status_ != status::StatusOK);

    return state_tracker_.num_active_sessions();
}

core::nanoseconds_t SenderSink::refresh(core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

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

sndio::ISink* SenderSink::to_sink() {
    return this;
}

sndio::ISource* SenderSink::to_source() {
    return NULL;
}

sndio::DeviceType SenderSink::type() const {
    return sndio::DeviceType_Sink;
}

sndio::DeviceState SenderSink::state() const {
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
    return sink_config_.input_sample_spec;
}

core::nanoseconds_t SenderSink::latency() const {
    return 0;
}

bool SenderSink::has_latency() const {
    return false;
}

bool SenderSink::has_clock() const {
    return sink_config_.enable_timing;
}

status::StatusCode SenderSink::write(audio::Frame& frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    return frame_writer_->write(frame);
}

} // namespace pipeline
} // namespace roc
