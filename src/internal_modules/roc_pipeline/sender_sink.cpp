/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_sink.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace pipeline {

SenderSink::SenderSink(const SenderSinkConfig& sink_config,
                       audio::ProcessorMap& processor_map,
                       rtp::EncodingMap& encoding_map,
                       core::IPool& packet_pool,
                       core::IPool& packet_buffer_pool,
                       core::IPool& frame_pool,
                       core::IPool& frame_buffer_pool,
                       core::IArena& arena)
    : IDevice(arena)
    , ISink(arena)
    , sink_config_(sink_config)
    , processor_map_(processor_map)
    , encoding_map_(encoding_map)
    , packet_factory_(packet_pool, packet_buffer_pool)
    , frame_factory_(frame_pool, frame_buffer_pool)
    , arena_(arena)
    , frame_writer_(NULL)
    , init_status_(status::NoStatus) {
    if (!sink_config_.deduce_defaults(processor_map)) {
        init_status_ = status::StatusBadConfig;
        return;
    }

    if (sink_config_.dumper.dump_file) {
        dumper_.reset(new (dumper_) dbgio::CsvDumper(sink_config_.dumper, arena));
        if ((init_status_ = dumper_->open()) != status::StatusOK) {
            return;
        }
    }

    audio::IFrameWriter* frm_writer = NULL;

    {
        const audio::SampleSpec inout_spec(sink_config_.input_sample_spec.sample_rate(),
                                           audio::PcmSubformat_Raw,
                                           sink_config_.input_sample_spec.channel_set());

        fanout_.reset(new (fanout_) audio::Fanout(inout_spec, frame_factory_, arena_));
        if ((init_status_ = fanout_->init_status()) != status::StatusOK) {
            return;
        }
        frm_writer = fanout_.get();
    }

    if (!sink_config_.input_sample_spec.is_raw()) {
        const audio::SampleSpec out_spec(sink_config_.input_sample_spec.sample_rate(),
                                         audio::PcmSubformat_Raw,
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

SenderSink::~SenderSink() {
    if (dumper_) {
        dumper_->close();
    }
}

status::StatusCode SenderSink::init_status() const {
    return init_status_;
}

SenderSlot* SenderSink::create_slot(const SenderSlotConfig& slot_config) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        // TODO(gh-183): return StatusBadState (control ops)
        return NULL;
    }

    roc_log(LogInfo, "sender sink: adding slot");

    core::SharedPtr<SenderSlot> slot = new (arena_) SenderSlot(
        sink_config_, slot_config, state_tracker_, processor_map_, encoding_map_,
        *fanout_, packet_factory_, frame_factory_, arena_, dumper_.get());

    if (!slot) {
        roc_log(LogError, "sender sink: can't create slot, allocation failed");
        // TODO(gh-183): return StatusNoMem (control ops)
        return NULL;
    }

    if (slot->init_status() != status::StatusOK) {
        roc_log(LogError,
                "sender sink: can't create slot, initialization failed: status=%s",
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

    return state_tracker_.num_sessions();
}

status::StatusCode SenderSink::refresh(core::nanoseconds_t current_time,
                                       core::nanoseconds_t* next_deadline) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    roc_panic_if_msg(current_time <= 0,
                     "sender sink: invalid timestamp:"
                     " expected positive value, got %lld",
                     (long long)current_time);

    for (core::SharedPtr<SenderSlot> slot = slots_.front(); slot;
         slot = slots_.nextof(*slot)) {
        core::nanoseconds_t slot_deadline = 0;

        const status::StatusCode code = slot->refresh(current_time, slot_deadline);
        if (code != status::StatusOK) {
            roc_log(LogError, "sender sink: failed to refresh slot: status=%s",
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

sndio::DeviceType SenderSink::type() const {
    return sndio::DeviceType_Sink;
}

sndio::ISink* SenderSink::to_sink() {
    return this;
}

sndio::ISource* SenderSink::to_source() {
    return NULL;
}

audio::SampleSpec SenderSink::sample_spec() const {
    return sink_config_.input_sample_spec;
}

core::nanoseconds_t SenderSink::frame_length() const {
    return 0;
}

bool SenderSink::has_state() const {
    return true;
}

sndio::DeviceState SenderSink::state() const {
    return state_tracker_.get_state();
}

status::StatusCode SenderSink::pause() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    return status::StatusOK;
}

status::StatusCode SenderSink::resume() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    return status::StatusOK;
}

bool SenderSink::has_latency() const {
    return false;
}

core::nanoseconds_t SenderSink::latency() const {
    return 0;
}

bool SenderSink::has_clock() const {
    return sink_config_.enable_cpu_clock;
}

status::StatusCode SenderSink::write(audio::Frame& frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (!state_tracker_.is_usable()) {
        return status::StatusBadState;
    }

    const status::StatusCode code = frame_writer_->write(frame);

    if (code != status::StatusOK) {
        roc_log(LogError, "sender sink: failed to write frame: status=%s",
                status::code_to_str(code));
        state_tracker_.set_broken();
    }

    return code;
}

status::StatusCode SenderSink::flush() {
    return status::StatusOK;
}

status::StatusCode SenderSink::close() {
    roc_panic_if(init_status_ != status::StatusOK);

    if (state_tracker_.is_closed()) {
        return status::StatusBadState;
    }

    state_tracker_.set_closed();

    return status::StatusOK;
}

void SenderSink::dispose() {
    arena_.dispose_object(*this);
}

} // namespace pipeline
} // namespace roc
