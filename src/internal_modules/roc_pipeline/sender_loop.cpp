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
    , writer_(NULL) {
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

SenderLoop::Tasks::CreateEndpoint::CreateEndpoint(SlotHandle slot,
                                                  address::Interface iface,
                                                  address::Protocol proto) {
    func_ = &SenderLoop::task_create_endpoint_;
    if (!slot) {
        roc_panic("sender sink: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
    iface_ = iface;
    proto_ = proto;
}

SenderLoop::EndpointHandle SenderLoop::Tasks::CreateEndpoint::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_);
    return (EndpointHandle)endpoint_;
}

SenderLoop::Tasks::SetEndpointDestinationWriter::SetEndpointDestinationWriter(
    EndpointHandle endpoint, packet::IWriter& writer) {
    func_ = &SenderLoop::task_set_endpoint_destination_writer_;
    if (!endpoint) {
        roc_panic("sender sink: endpoint handle is null");
    }
    endpoint_ = (SenderEndpoint*)endpoint;
    writer_ = &writer;
}

SenderLoop::Tasks::SetEndpointDestinationAddress::SetEndpointDestinationAddress(
    EndpointHandle endpoint, const address::SocketAddr& addr) {
    func_ = &SenderLoop::task_set_endpoint_destination_address_;
    if (!endpoint) {
        roc_panic("sender sink: endpoint handle is null");
    }
    endpoint_ = (SenderEndpoint*)endpoint;
    addr_ = addr;
}

SenderLoop::Tasks::CheckSlotIsReady::CheckSlotIsReady(SlotHandle slot) {
    func_ = &SenderLoop::task_check_slot_is_ready_;
    if (!slot) {
        roc_panic("sender sink: slot handle is null");
    }
    slot_ = (SenderSlot*)slot;
}

SenderLoop::SenderLoop(IPipelineTaskScheduler& scheduler,
                       const SenderConfig& config,
                       const rtp::FormatMap& format_map,
                       packet::PacketFactory& packet_factory,
                       core::BufferFactory<uint8_t>& byte_buffer_factory,
                       core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                       core::IAllocator& allocator)
    : PipelineLoop(scheduler, config.tasks, config.input_sample_spec)
    , sink_(config,
            format_map,
            packet_factory,
            byte_buffer_factory,
            sample_buffer_factory,
            allocator)
    , timestamp_(0)
    , valid_(false) {
    if (!sink_.valid()) {
        return;
    }

    if (config.timing) {
        ticker_.reset(new (ticker_) core::Ticker(config.input_sample_spec.sample_rate()));
        if (!ticker_) {
            return;
        }
    }

    valid_ = true;
}

bool SenderLoop::valid() const {
    return valid_;
}

sndio::ISink& SenderLoop::sink() {
    roc_panic_if_not(valid());

    return *this;
}

sndio::DeviceType SenderLoop::type() const {
    roc_panic_if(!valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.type();
}

sndio::DeviceState SenderLoop::state() const {
    roc_panic_if(!valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.state();
}

void SenderLoop::pause() {
    roc_panic_if(!valid());

    core::Mutex::Lock lock(sink_mutex_);

    sink_.pause();
}

bool SenderLoop::resume() {
    roc_panic_if(!valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.resume();
}

bool SenderLoop::restart() {
    roc_panic_if(!valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.restart();
}

audio::SampleSpec SenderLoop::sample_spec() const {
    roc_panic_if_not(valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.sample_spec();
}

core::nanoseconds_t SenderLoop::latency() const {
    roc_panic_if_not(valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.latency();
}

bool SenderLoop::has_clock() const {
    roc_panic_if_not(valid());

    core::Mutex::Lock lock(sink_mutex_);

    return sink_.has_clock();
}

void SenderLoop::write(audio::Frame& frame) {
    roc_panic_if_not(valid());

    core::Mutex::Lock lock(sink_mutex_);

    if (ticker_) {
        ticker_->wait(timestamp_);
    }

    // Invokes process_subframe_imp() and process_task_imp().
    if (!process_subframes_and_tasks(frame)) {
        return;
    }

    timestamp_ +=
        packet::timestamp_t(frame.num_samples() / sink_.sample_spec().num_channels());
}

core::nanoseconds_t SenderLoop::timestamp_imp() const {
    return core::timestamp(core::ClockMonotonic);
}

bool SenderLoop::process_subframe_imp(audio::Frame& frame) {
    sink_.write(frame);

    if (sink_.get_update_deadline() <= core::timestamp(core::ClockMonotonic)) {
        sink_.update();
    }

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

bool SenderLoop::task_create_endpoint_(Task& task) {
    roc_panic_if(!task.slot_);

    task.endpoint_ = task.slot_->create_endpoint(task.iface_, task.proto_);
    return (bool)task.endpoint_;
}

bool SenderLoop::task_set_endpoint_destination_writer_(Task& task) {
    roc_panic_if(!task.endpoint_);
    roc_panic_if(!task.writer_);

    task.endpoint_->set_destination_writer(*task.writer_);
    return true;
}

bool SenderLoop::task_set_endpoint_destination_address_(Task& task) {
    roc_panic_if(!task.endpoint_);

    task.endpoint_->set_destination_address(task.addr_);
    return true;
}

bool SenderLoop::task_check_slot_is_ready_(Task& task) {
    roc_panic_if(!task.slot_);

    return task.slot_->is_ready();
}

} // namespace pipeline
} // namespace roc
