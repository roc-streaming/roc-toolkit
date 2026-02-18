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
#include "roc_status/code_to_str.h"

namespace roc {
namespace pipeline {

ReceiverSource::ReceiverSource(const ReceiverSourceConfig& source_config,
                               audio::ProcessorMap& processor_map,
                               rtp::EncodingMap& encoding_map,
                               core::IPool& packet_pool,
                               core::IPool& packet_buffer_pool,
                               core::IPool& frame_pool,
                               core::IPool& frame_buffer_pool,
                               core::IArena& arena)
    : IDevice(arena)
    , ISource(arena)
    , source_config_(source_config)
    , processor_map_(processor_map)
    , encoding_map_(encoding_map)
    , packet_factory_(packet_pool, packet_buffer_pool)
    , frame_factory_(frame_pool, frame_buffer_pool)
    , arena_(arena)
    , frame_reader_(NULL)
    , init_status_(status::NoStatus) {
    if (!source_config_.deduce_defaults(processor_map)) {
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (source_config.common.dumper.dump_file) {
        dumper_.reset(new (dumper_) dbgio::CsvDumper(source_config.common.dumper, arena));
        if ((init_status_ = dumper_->open()) != status::StatusOK) {
            return;
        }
    }

    audio::IFrameReader* frm_reader = NULL;

    {
        const audio::SampleSpec inout_spec(
            source_config_.common.output_sample_spec.sample_rate(),
            audio::PcmSubformat_Raw,
            source_config_.common.output_sample_spec.channel_set());

        mixer_.reset(new (mixer_) audio::Mixer(inout_spec, true, frame_factory_, arena));
        if ((init_status_ = mixer_->init_status()) != status::StatusOK) {
            return;
        }
        frm_reader = mixer_.get();
    }

    if (!source_config_.common.output_sample_spec.is_raw()) {
        const audio::SampleSpec in_spec(
            source_config_.common.output_sample_spec.sample_rate(),
            audio::PcmSubformat_Raw,
            source_config_.common.output_sample_spec.channel_set());

        pcm_mapper_.reset(new (pcm_mapper_) audio::PcmMapperReader(
            *frm_reader, frame_factory_, in_spec,
            source_config_.common.output_sample_spec));
        if ((init_status_ = pcm_mapper_->init_status()) != status::StatusOK) {
            return;
        }
        frm_reader = pcm_mapper_.get();
    }

    if (source_config_.common.enable_profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingReader(
            *frm_reader, arena, source_config_.common.output_sample_spec,
            source_config_.common.profiler));
        if ((init_status_ = profiler_->init_status()) != status::StatusOK) {
            return;
        }
        frm_reader = profiler_.get();
    }

    frame_reader_ = frm_reader;
    init_status_ = status::StatusOK;
}

ReceiverSource::~ReceiverSource() {
    if (dumper_) {
        dumper_->close();
    }
}

status::StatusCode ReceiverSource::init_status() const {
    return init_status_;
}

ReceiverSlot* ReceiverSource::create_slot(const ReceiverSlotConfig& slot_config) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        // TODO(gh-183): return StatusBadState (control ops)
        return NULL;
    }

    roc_log(LogInfo, "receiver source: adding slot");

    core::SharedPtr<ReceiverSlot> slot = new (arena_) ReceiverSlot(
        source_config_, slot_config, state_tracker_, *mixer_, processor_map_,
        encoding_map_, packet_factory_, frame_factory_, arena_, dumper_.get());

    if (!slot) {
        roc_log(LogError, "receiver source: can't create slot, allocation failed");
        // TODO(gh-183): return StatusNoMem (control ops)
        return NULL;
    }

    if (slot->init_status() != status::StatusOK) {
        roc_log(LogError,
                "receiver source: can't create slot, initialization failed: status=%s",
                status::code_to_str(slot->init_status()));
        // TODO(gh-183): forward status (control ops)
        return NULL;
    }

    slots_.push_back(*slot);
    return slot.get();
}

void ReceiverSource::delete_slot(ReceiverSlot* slot) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_log(LogInfo, "receiver source: removing slot");

    slots_.remove(*slot);
}

size_t ReceiverSource::num_sessions() const {
    return state_tracker_.num_sessions();
}

status::StatusCode ReceiverSource::refresh(core::nanoseconds_t current_time,
                                           core::nanoseconds_t* next_deadline) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    roc_panic_if_msg(current_time <= 0,
                     "receiver source: invalid timestamp:"
                     " expected positive value, got %lld",
                     (long long)current_time);

    for (core::SharedPtr<ReceiverSlot> slot = slots_.front(); slot;
         slot = slots_.nextof(*slot)) {
        core::nanoseconds_t slot_deadline = 0;

        const status::StatusCode code = slot->refresh(current_time, slot_deadline);
        if (code != status::StatusOK) {
            roc_log(LogError, "receiver source: failed to refresh slot: status=%s",
                    status::code_to_str(code));
            state_tracker_.set_broken();
            return code;
        }

        if (next_deadline && slot_deadline != 0) {
            *next_deadline = *next_deadline == 0
                ? slot_deadline
                : std::min(*next_deadline, slot_deadline);
        }
    }

    return status::StatusOK;
}

sndio::DeviceType ReceiverSource::type() const {
    return sndio::DeviceType_Source;
}

sndio::ISink* ReceiverSource::to_sink() {
    return NULL;
}

sndio::ISource* ReceiverSource::to_source() {
    return this;
}

audio::SampleSpec ReceiverSource::sample_spec() const {
    return source_config_.common.output_sample_spec;
}

core::nanoseconds_t ReceiverSource::frame_length() const {
    return 0;
}

bool ReceiverSource::has_state() const {
    return true;
}

sndio::DeviceState ReceiverSource::state() const {
    return state_tracker_.get_state();
}

status::StatusCode ReceiverSource::pause() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    return status::StatusOK;
}

status::StatusCode ReceiverSource::resume() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    return status::StatusOK;
}

bool ReceiverSource::has_latency() const {
    return false;
}

core::nanoseconds_t ReceiverSource::latency() const {
    return 0;
}

bool ReceiverSource::has_clock() const {
    return source_config_.common.enable_cpu_clock;
}

status::StatusCode ReceiverSource::rewind() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    return status::StatusOK;
}

void ReceiverSource::reclock(core::nanoseconds_t playback_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if_msg(playback_time <= 0,
                     "receiver source: invalid timestamp:"
                     " expected positive value, got %lld",
                     (long long)playback_time);

    for (core::SharedPtr<ReceiverSlot> slot = slots_.front(); slot;
         slot = slots_.nextof(*slot)) {
        slot->reclock(playback_time);
    }
}

status::StatusCode ReceiverSource::read(audio::Frame& frame,
                                        packet::stream_timestamp_t duration,
                                        audio::FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    const status::StatusCode code = frame_reader_->read(frame, duration, mode);

    if (code != status::StatusOK && code != status::StatusPart
        && code != status::StatusDrain) {
        roc_log(LogError, "receiver source: failed to read frame: status=%s",
                status::code_to_str(code));
        state_tracker_.set_broken();
    }

    return code;
}

status::StatusCode ReceiverSource::close() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (state_tracker_.is_closed()) {
        return status::StatusBadState;
    }

    state_tracker_.set_closed();

    return status::StatusOK;
}

void ReceiverSource::dispose() {
    arena_.dispose_object(*this);
}

} // namespace pipeline
} // namespace roc
