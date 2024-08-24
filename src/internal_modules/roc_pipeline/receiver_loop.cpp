/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_loop.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/thread.h"

namespace roc {
namespace pipeline {

ReceiverLoop::Task::Task()
    : func_(NULL)
    , slot_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , inbound_address_()
    , inbound_writer_(NULL)
    , outbound_writer_(NULL)
    , slot_metrics_(NULL)
    , party_metrics_(NULL)
    , party_count_(NULL) {
}

ReceiverLoop::Tasks::CreateSlot::CreateSlot(const ReceiverSlotConfig& slot_config) {
    func_ = &ReceiverLoop::task_create_slot_;
    slot_config_ = slot_config;
}

ReceiverLoop::SlotHandle ReceiverLoop::Tasks::CreateSlot::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(slot_);
    return (SlotHandle)slot_;
}

ReceiverLoop::Tasks::DeleteSlot::DeleteSlot(SlotHandle slot) {
    func_ = &ReceiverLoop::task_delete_slot_;
    if (!slot) {
        roc_panic("receiver loop: slot handle is null");
    }
    slot_ = (ReceiverSlot*)slot;
}

ReceiverLoop::Tasks::QuerySlot::QuerySlot(SlotHandle slot,
                                          ReceiverSlotMetrics& slot_metrics,
                                          ReceiverParticipantMetrics* party_metrics,
                                          size_t* party_count) {
    func_ = &ReceiverLoop::task_query_slot_;
    if (!slot) {
        roc_panic("receiver loop: slot handle is null");
    }
    slot_ = (ReceiverSlot*)slot;
    slot_metrics_ = &slot_metrics;
    party_metrics_ = party_metrics;
    party_count_ = party_count;
}

ReceiverLoop::Tasks::AddEndpoint::AddEndpoint(SlotHandle slot,
                                              address::Interface iface,
                                              address::Protocol proto,
                                              const address::SocketAddr& inbound_address,
                                              packet::IWriter* outbound_writer) {
    func_ = &ReceiverLoop::task_add_endpoint_;
    if (!slot) {
        roc_panic("receiver loop: slot handle is null");
    }
    slot_ = (ReceiverSlot*)slot;
    iface_ = iface;
    proto_ = proto;
    inbound_address_ = inbound_address;
    outbound_writer_ = outbound_writer;
}

packet::IWriter* ReceiverLoop::Tasks::AddEndpoint::get_inbound_writer() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(inbound_writer_);
    return inbound_writer_;
}

ReceiverLoop::ReceiverLoop(IPipelineTaskScheduler& scheduler,
                           const ReceiverSourceConfig& source_config,
                           audio::ProcessorMap& processor_map,
                           rtp::EncodingMap& encoding_map,
                           core::IPool& packet_pool,
                           core::IPool& packet_buffer_pool,
                           core::IPool& frame_pool,
                           core::IPool& frame_buffer_pool,
                           core::IArena& arena)
    : IDevice(arena)
    , PipelineLoop(scheduler,
                   source_config.pipeline_loop,
                   source_config.common.output_sample_spec,
                   frame_pool,
                   frame_buffer_pool,
                   Dir_ReadFrames)
    , ISource(arena)
    , source_(source_config,
              processor_map,
              encoding_map,
              packet_pool,
              packet_buffer_pool,
              frame_pool,
              frame_buffer_pool,
              arena)
    , ticker_ts_(0)
    , auto_reclock_(source_config.common.enable_auto_reclock)
    , init_status_(status::NoStatus) {
    if ((init_status_ = source_.init_status()) != status::StatusOK) {
        return;
    }

    if (source_config.common.enable_cpu_clock) {
        ticker_.reset(new (ticker_) core::Ticker(
            source_config.common.output_sample_spec.sample_rate()));
    }

    init_status_ = status::StatusOK;
}

ReceiverLoop::~ReceiverLoop() {
}

status::StatusCode ReceiverLoop::init_status() const {
    return init_status_;
}

sndio::ISource& ReceiverLoop::source() {
    return *this;
}

sndio::DeviceType ReceiverLoop::type() const {
    core::Mutex::Lock lock(source_mutex_);

    return source_.type();
}

sndio::ISink* ReceiverLoop::to_sink() {
    return NULL;
}

sndio::ISource* ReceiverLoop::to_source() {
    return this;
}

audio::SampleSpec ReceiverLoop::sample_spec() const {
    core::Mutex::Lock lock(source_mutex_);

    return source_.sample_spec();
}

core::nanoseconds_t ReceiverLoop::frame_length() const {
    core::Mutex::Lock lock(source_mutex_);

    return source_.frame_length();
}

bool ReceiverLoop::has_state() const {
    core::Mutex::Lock lock(source_mutex_);

    return source_.has_state();
}

sndio::DeviceState ReceiverLoop::state() const {
    core::Mutex::Lock lock(source_mutex_);

    return source_.state();
}

status::StatusCode ReceiverLoop::pause() {
    core::Mutex::Lock lock(source_mutex_);

    return source_.pause();
}

status::StatusCode ReceiverLoop::resume() {
    core::Mutex::Lock lock(source_mutex_);

    return source_.resume();
}

bool ReceiverLoop::has_latency() const {
    core::Mutex::Lock lock(source_mutex_);

    return source_.has_latency();
}

core::nanoseconds_t ReceiverLoop::latency() const {
    core::Mutex::Lock lock(source_mutex_);

    return source_.latency();
}

bool ReceiverLoop::has_clock() const {
    core::Mutex::Lock lock(source_mutex_);

    return source_.has_clock();
}

status::StatusCode ReceiverLoop::rewind() {
    core::Mutex::Lock lock(source_mutex_);

    return source_.rewind();
}

void ReceiverLoop::reclock(core::nanoseconds_t timestamp) {
    roc_panic_if(init_status_ != status::StatusOK);

    if (auto_reclock_) {
        roc_panic("receiver loop: unexpected reclock() call in auto-reclock mode");
    }

    core::Mutex::Lock lock(source_mutex_);

    source_.reclock(timestamp);
}

status::StatusCode ReceiverLoop::read(audio::Frame& frame,
                                      packet::stream_timestamp_t duration,
                                      audio::FrameReadMode mode) {
    roc_panic_if(init_status_ != status::StatusOK);

    core::Mutex::Lock lock(source_mutex_);

    if (source_.state() == sndio::DeviceState_Broken) {
        // Don't go to sleep if we're broke.
        return status::StatusBadState;
    }

    if (ticker_) {
        ticker_->wait(ticker_ts_);
    }

    // Invokes process_subframe_imp() and process_task_imp().
    const status::StatusCode code = process_subframes_and_tasks(frame, duration, mode);

    roc_panic_if_msg(code <= status::NoStatus || code >= status::MaxStatus,
                     "receiver loop: invalid status code %d", code);

    if (code == status::StatusOK || code == status::StatusPart) {
        ticker_ts_ += frame.duration();

        if (auto_reclock_) {
            source_.reclock(core::timestamp(core::ClockUnix));
        }
    }

    return code;
}

status::StatusCode ReceiverLoop::close() {
    core::Mutex::Lock lock(source_mutex_);

    return source_.close();
}

void ReceiverLoop::dispose() {
    arena().dispose_object(*this);
}

core::nanoseconds_t ReceiverLoop::timestamp_imp() const {
    return core::timestamp(core::ClockMonotonic);
}

uint64_t ReceiverLoop::tid_imp() const {
    return core::Thread::get_tid();
}

status::StatusCode ReceiverLoop::process_subframe_imp(audio::Frame& frame,
                                                      packet::stream_timestamp_t duration,
                                                      audio::FrameReadMode mode) {
    status::StatusCode code = status::NoStatus;

    // TODO(gh-674): handle returned deadline and schedule refresh
    core::nanoseconds_t next_deadline = 0;

    if ((code = source_.refresh(core::timestamp(core::ClockUnix), &next_deadline))
        != status::StatusOK) {
        return code;
    }

    if ((code = source_.read(frame, duration, mode)) != status::StatusOK) {
        return code;
    }

    return status::StatusOK;
}

bool ReceiverLoop::process_task_imp(PipelineTask& basic_task) {
    Task& task = (Task&)basic_task;

    roc_panic_if_not(task.func_);
    return (this->*(task.func_))(task);
}

bool ReceiverLoop::task_create_slot_(Task& task) {
    task.slot_ = source_.create_slot(task.slot_config_);
    return (bool)task.slot_;
}

bool ReceiverLoop::task_delete_slot_(Task& task) {
    roc_panic_if(!task.slot_);

    source_.delete_slot(task.slot_);
    return true;
}

bool ReceiverLoop::task_query_slot_(Task& task) {
    roc_panic_if(!task.slot_);
    roc_panic_if(!task.slot_metrics_);

    task.slot_->get_metrics(*task.slot_metrics_, task.party_metrics_, task.party_count_);
    return true;
}

bool ReceiverLoop::task_add_endpoint_(Task& task) {
    roc_panic_if(!task.slot_);

    ReceiverEndpoint* endpoint = task.slot_->add_endpoint(
        task.iface_, task.proto_, task.inbound_address_, task.outbound_writer_);
    if (!endpoint) {
        return false;
    }
    task.inbound_writer_ = &endpoint->inbound_writer();
    return true;
}

} // namespace pipeline
} // namespace roc
