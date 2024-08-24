/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_loop.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/thread.h"
#include "roc_pipeline/sender_endpoint.h"

namespace roc {
namespace pipeline {

SenderLoop::Task::Task()
    : func_(NULL)
    , slot_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , outbound_writer_(NULL)
    , inbound_writer_(NULL)
    , slot_metrics_(NULL)
    , party_metrics_(NULL)
    , party_count_(NULL) {
}

SenderLoop::Tasks::CreateSlot::CreateSlot(const SenderSlotConfig& slot_config) {
    func_ = &SenderLoop::task_create_slot_;
    slot_config_ = slot_config;
}

SenderLoop::SlotHandle SenderLoop::Tasks::CreateSlot::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(slot_);
    return (SlotHandle)slot_;
}

SenderLoop::Tasks::DeleteSlot::DeleteSlot(SlotHandle slot) {
    func_ = &SenderLoop::task_delete_slot_;
    if (!slot) {
        roc_panic("sender loop: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
}

SenderLoop::Tasks::QuerySlot::QuerySlot(SlotHandle slot,
                                        SenderSlotMetrics& slot_metrics,
                                        SenderParticipantMetrics* party_metrics,
                                        size_t* party_count) {
    func_ = &SenderLoop::task_query_slot_;
    if (!slot) {
        roc_panic("sender loop: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
    slot_metrics_ = &slot_metrics;
    party_metrics_ = party_metrics;
    party_count_ = party_count;
}

SenderLoop::Tasks::AddEndpoint::AddEndpoint(SlotHandle slot,
                                            address::Interface iface,
                                            address::Protocol proto,
                                            const address::SocketAddr& outbound_address,
                                            packet::IWriter& outbound_writer) {
    func_ = &SenderLoop::task_add_endpoint_;
    if (!slot) {
        roc_panic("sender loop: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
    iface_ = iface;
    proto_ = proto;
    outbound_address_ = outbound_address;
    outbound_writer_ = &outbound_writer;
}

packet::IWriter* SenderLoop::Tasks::AddEndpoint::get_inbound_writer() const {
    if (!success()) {
        return NULL;
    }
    return inbound_writer_;
}

SenderLoop::SenderLoop(IPipelineTaskScheduler& scheduler,
                       const SenderSinkConfig& sink_config,
                       audio::ProcessorMap& processor_map,
                       rtp::EncodingMap& encoding_map,
                       core::IPool& packet_pool,
                       core::IPool& packet_buffer_pool,
                       core::IPool& frame_pool,
                       core::IPool& frame_buffer_pool,
                       core::IArena& arena)
    : IDevice(arena)
    , PipelineLoop(scheduler,
                   sink_config.pipeline_loop,
                   sink_config.input_sample_spec,
                   frame_pool,
                   frame_buffer_pool,
                   Dir_WriteFrames)
    , ISink(arena)
    , sink_(sink_config,
            processor_map,
            encoding_map,
            packet_pool,
            packet_buffer_pool,
            frame_pool,
            frame_buffer_pool,
            arena)
    , ticker_ts_(0)
    , auto_cts_(sink_config.enable_auto_cts)
    , sample_spec_(sink_config.input_sample_spec)
    , init_status_(status::NoStatus) {
    if ((init_status_ = sink_.init_status()) != status::StatusOK) {
        return;
    }

    if (sink_config.enable_cpu_clock) {
        ticker_.reset(new (ticker_)
                          core::Ticker(sink_config.input_sample_spec.sample_rate()));
    }

    init_status_ = status::StatusOK;
}

SenderLoop::~SenderLoop() {
}

status::StatusCode SenderLoop::init_status() const {
    return init_status_;
}

sndio::ISink& SenderLoop::sink() {
    return *this;
}

sndio::DeviceType SenderLoop::type() const {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.type();
}

sndio::ISink* SenderLoop::to_sink() {
    return this;
}

sndio::ISource* SenderLoop::to_source() {
    return NULL;
}

audio::SampleSpec SenderLoop::sample_spec() const {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.sample_spec();
}

core::nanoseconds_t SenderLoop::frame_length() const {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.frame_length();
}

bool SenderLoop::has_state() const {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.has_state();
}

sndio::DeviceState SenderLoop::state() const {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.state();
}

status::StatusCode SenderLoop::pause() {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.pause();
}

status::StatusCode SenderLoop::resume() {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.resume();
}

bool SenderLoop::has_latency() const {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.has_latency();
}

core::nanoseconds_t SenderLoop::latency() const {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.latency();
}

bool SenderLoop::has_clock() const {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.has_clock();
}

status::StatusCode SenderLoop::write(audio::Frame& frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (auto_cts_) {
        if (frame.capture_timestamp() != 0) {
            roc_panic("sender loop: unexpected non-zero cts in auto-cts mode");
        }
        frame.set_capture_timestamp(core::timestamp(core::ClockUnix));
    }

    core::Mutex::Lock lock(sink_mutex_);

    if (sink_.state() == sndio::DeviceState_Broken) {
        // Don't go to sleep if we're broke.
        return status::StatusBadState;
    }

    if (ticker_) {
        ticker_->wait(ticker_ts_);
        ticker_ts_ += frame.duration();
    }

    // Invokes process_subframe_imp() and process_task_imp().
    const status::StatusCode code =
        process_subframes_and_tasks(frame, frame.duration(), audio::ModeHard);

    roc_panic_if_msg(code <= status::NoStatus || code >= status::MaxStatus,
                     "sender loop: invalid status code %d", code);

    return code;
}

status::StatusCode SenderLoop::flush() {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.flush();
}

status::StatusCode SenderLoop::close() {
    core::Mutex::Lock lock(sink_mutex_);

    return sink_.close();
}

void SenderLoop::dispose() {
    arena().dispose_object(*this);
}

core::nanoseconds_t SenderLoop::timestamp_imp() const {
    return core::timestamp(core::ClockMonotonic);
}

uint64_t SenderLoop::tid_imp() const {
    return core::Thread::get_tid();
}

status::StatusCode SenderLoop::process_subframe_imp(audio::Frame& frame,
                                                    packet::stream_timestamp_t,
                                                    audio::FrameReadMode) {
    status::StatusCode code = status::NoStatus;

    if ((code = sink_.write(frame)) != status::StatusOK) {
        return code;
    }

    // TODO(gh-674): handle returned deadline and schedule refresh
    core::nanoseconds_t next_deadline = 0;

    if ((code = sink_.refresh(core::timestamp(core::ClockUnix), &next_deadline))
        != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

bool SenderLoop::process_task_imp(PipelineTask& basic_task) {
    Task& task = (Task&)basic_task;

    roc_panic_if_not(task.func_);
    return (this->*(task.func_))(task);
}

bool SenderLoop::task_create_slot_(Task& task) {
    task.slot_ = sink_.create_slot(task.slot_config_);
    return (bool)task.slot_;
}

bool SenderLoop::task_delete_slot_(Task& task) {
    roc_panic_if(!task.slot_);

    sink_.delete_slot(task.slot_);
    return true;
}

bool SenderLoop::task_query_slot_(Task& task) {
    roc_panic_if(!task.slot_);
    roc_panic_if(!task.slot_metrics_);

    task.slot_->get_metrics(*task.slot_metrics_, task.party_metrics_, task.party_count_);
    return true;
}

bool SenderLoop::task_add_endpoint_(Task& task) {
    roc_panic_if(!task.slot_);

    SenderEndpoint* endpoint = task.slot_->add_endpoint(
        task.iface_, task.proto_, task.outbound_address_, *task.outbound_writer_);
    if (!endpoint) {
        return false;
    }
    task.inbound_writer_ = endpoint->inbound_writer();
    return true;
}

} // namespace pipeline
} // namespace roc
