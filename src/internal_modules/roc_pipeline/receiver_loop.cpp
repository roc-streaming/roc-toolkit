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
#include "roc_core/shared_ptr.h"

namespace roc {
namespace pipeline {

ReceiverLoop::Task::Task()
    : func_(NULL)
    , slot_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , writer_(NULL)
    , slot_metrics_(NULL)
    , sess_metrics_(NULL)
    , sess_metrics_size_(NULL) {
}

ReceiverLoop::Tasks::CreateSlot::CreateSlot() {
    func_ = &ReceiverLoop::task_create_slot_;
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
                                          ReceiverSessionMetrics* sess_metrics,
                                          size_t* sess_metrics_size) {
    func_ = &ReceiverLoop::task_query_slot_;
    if (!slot) {
        roc_panic("receiver loop: slot handle is null");
    }
    slot_ = (ReceiverSlot*)slot;
    slot_metrics_ = &slot_metrics;
    sess_metrics_ = sess_metrics;
    sess_metrics_size_ = sess_metrics_size;
}

ReceiverLoop::Tasks::AddEndpoint::AddEndpoint(SlotHandle slot,
                                              address::Interface iface,
                                              address::Protocol proto) {
    func_ = &ReceiverLoop::task_add_endpoint_;
    if (!slot) {
        roc_panic("receiver loop: slot handle is null");
    }
    slot_ = (ReceiverSlot*)slot;
    iface_ = iface;
    proto_ = proto;
}

packet::IWriter* ReceiverLoop::Tasks::AddEndpoint::get_writer() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(writer_);
    return writer_;
}

ReceiverLoop::ReceiverLoop(IPipelineTaskScheduler& scheduler,
                           const ReceiverConfig& config,
                           const rtp::FormatMap& format_map,
                           packet::PacketFactory& packet_factory,
                           core::BufferFactory<uint8_t>& byte_buffer_factory,
                           core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                           core::IArena& arena)
    : PipelineLoop(scheduler, config.tasks, config.common.output_sample_spec)
    , source_(config,
              format_map,
              packet_factory,
              byte_buffer_factory,
              sample_buffer_factory,
              arena)
    , timestamp_(0)
    , valid_(false) {
    if (!source_.is_valid()) {
        return;
    }

    if (config.common.enable_timing) {
        ticker_.reset(new (ticker_)
                          core::Ticker(config.common.output_sample_spec.sample_rate()));
        if (!ticker_) {
            return;
        }
    }

    valid_ = true;
}

bool ReceiverLoop::is_valid() const {
    return valid_;
}

sndio::ISource& ReceiverLoop::source() {
    roc_panic_if(!is_valid());

    return *this;
}

sndio::DeviceType ReceiverLoop::type() const {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    return source_.type();
}

sndio::DeviceState ReceiverLoop::state() const {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    return source_.state();
}

void ReceiverLoop::pause() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    source_.pause();
}

bool ReceiverLoop::resume() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    return source_.resume();
}

bool ReceiverLoop::restart() {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    return source_.restart();
}

audio::SampleSpec ReceiverLoop::sample_spec() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(source_mutex_);

    return source_.sample_spec();
}

core::nanoseconds_t ReceiverLoop::latency() const {
    roc_panic_if_not(is_valid());

    core::Mutex::Lock lock(source_mutex_);

    return source_.latency();
}

bool ReceiverLoop::has_latency() const {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    return source_.has_latency();
}

bool ReceiverLoop::has_clock() const {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    return source_.has_clock();
}

void ReceiverLoop::reclock(core::nanoseconds_t timestamp) {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    source_.reclock(timestamp);
}

bool ReceiverLoop::read(audio::Frame& frame) {
    roc_panic_if(!is_valid());

    core::Mutex::Lock lock(source_mutex_);

    if (ticker_) {
        ticker_->wait(timestamp_);
    }

    // Invokes process_subframe_imp() and process_task_imp().
    if (!process_subframes_and_tasks(frame)) {
        return false;
    }

    timestamp_ +=
        packet::timestamp_t(frame.num_samples() / source_.sample_spec().num_channels());

    return true;
}

core::nanoseconds_t ReceiverLoop::timestamp_imp() const {
    return core::timestamp(core::ClockMonotonic);
}

bool ReceiverLoop::process_subframe_imp(audio::Frame& frame) {
    return source_.read(frame);
}

bool ReceiverLoop::process_task_imp(PipelineTask& basic_task) {
    Task& task = (Task&)basic_task;

    roc_panic_if_not(task.func_);
    return (this->*(task.func_))(task);
}

bool ReceiverLoop::task_create_slot_(Task& task) {
    task.slot_ = source_.create_slot();
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

    task.slot_->get_metrics(*task.slot_metrics_, task.sess_metrics_,
                            task.sess_metrics_size_);
    return true;
}

bool ReceiverLoop::task_add_endpoint_(Task& task) {
    roc_panic_if(!task.slot_);

    ReceiverEndpoint* endpoint = task.slot_->add_endpoint(task.iface_, task.proto_);
    if (!endpoint) {
        return false;
    }
    task.writer_ = &endpoint->writer();
    return true;
}

} // namespace pipeline
} // namespace roc
