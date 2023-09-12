/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_loop.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

SenderLoop::Task::Task()
    : func_(NULL)
    , slot_(NULL)
    , endpoint_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , writer_(NULL)
    , slot_metrics_(NULL)
    , sess_metrics_(NULL) {
}

SenderLoop::Tasks::CreateSlot::CreateSlot() {
    func_ = &SenderLoop::task_create_slot_;
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
                                        SenderSessionMetrics* sess_metrics) {
    func_ = &SenderLoop::task_query_slot_;
    if (!slot) {
        roc_panic("sender loop: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
    slot_metrics_ = &slot_metrics;
    sess_metrics_ = sess_metrics;
}

SenderLoop::Tasks::AddEndpoint::AddEndpoint(SlotHandle slot,
                                            address::Interface iface,
                                            address::Protocol proto,
                                            const address::SocketAddr& dest_address,
                                            packet::IWriter& dest_writer) {
    func_ = &SenderLoop::task_add_endpoint_;
    if (!slot) {
        roc_panic("sender loop: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
    iface_ = iface;
    proto_ = proto;
    address_ = dest_address;
    writer_ = &dest_writer;
}

SenderLoop::EndpointHandle SenderLoop::Tasks::AddEndpoint::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_);
    return (EndpointHandle)endpoint_;
}

SenderLoop::SenderLoop(IPipelineTaskScheduler& scheduler,
                       const SenderConfig& config,
                       const rtp::FormatMap& format_map,
                       packet::PacketFactory& packet_factory,
                       core::BufferFactory<uint8_t>& byte_buffer_factory,
                       core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                       core::IArena& arena)
    : PipelineLoop(scheduler, config.tasks, config.input_sample_spec)
    , sink_(config,
            format_map,
            packet_factory,
            byte_buffer_factory,
            sample_buffer_factory,
            arena)
    , ticker_ts_(0)
    , valid_(false) {
    if (!sink_.is_valid()) {
        return;
    }

    if (config.enable_timing) {
        ticker_.reset(new (ticker_) core::Ticker(config.input_sample_spec.sample_rate()));
        if (!ticker_) {
            return;
        }
    }

    valid_ = true;
}

bool SenderLoop::is_valid() const {
    return valid_;
}

sndio::ISink& SenderLoop::sink() {
    roc_panic_if_not(is_valid());

    return *this;
}

sndio::DeviceType SenderLoop::type() const {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.type();
}

sndio::DeviceState SenderLoop::state() const {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.state();
}

void SenderLoop::pause() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    sink_.pause();
}

bool SenderLoop::resume() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.resume();
}

bool SenderLoop::restart() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.restart();
}

audio::SampleSpec SenderLoop::sample_spec() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.sample_spec();
}

core::nanoseconds_t SenderLoop::latency() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.latency();
}

bool SenderLoop::has_latency() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.has_latency();
}

bool SenderLoop::has_clock() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.has_clock();
}

void SenderLoop::write(audio::Frame& frame) {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(sink_mutex_);

    if (ticker_) {
        ticker_->wait(ticker_ts_);
        ticker_ts_ += frame.num_samples() / sink_.sample_spec().num_channels();
    }

    // invokes process_subframe_imp() and process_task_imp()
    if (!process_subframes_and_tasks(frame)) {
        return;
    }
}

core::nanoseconds_t SenderLoop::timestamp_imp() const {
    return core::timestamp(core::ClockMonotonic);
}

bool SenderLoop::process_subframe_imp(audio::Frame& frame) {
    sink_.write(frame);

    // TODO: handle returned deadline and schedule refresh
    sink_.refresh(core::timestamp(core::ClockUnix));

    return true;
}

bool SenderLoop::process_task_imp(PipelineTask& basic_task) {
    Task& task = (Task&)basic_task;

    roc_panic_if_not(task.func_);
    return (this->*(task.func_))(task);
}

bool SenderLoop::task_create_slot_(Task& task) {
    task.slot_ = sink_.create_slot();
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

    task.slot_->get_metrics(*task.slot_metrics_, task.sess_metrics_);
    return true;
}

bool SenderLoop::task_add_endpoint_(Task& task) {
    roc_panic_if(!task.slot_);

    task.endpoint_ =
        task.slot_->add_endpoint(task.iface_, task.proto_, task.address_, *task.writer_);
    if (!task.endpoint_) {
        return false;
    }

    return true;
}

} // namespace pipeline
} // namespace roc
