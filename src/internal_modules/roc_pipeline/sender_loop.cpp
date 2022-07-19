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
    , endpoint_set_(NULL)
    , endpoint_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , writer_(NULL) {
}

SenderLoop::Tasks::CreateEndpointSet::CreateEndpointSet() {
    func_ = &SenderLoop::task_create_endpoint_set_;
}

SenderLoop::EndpointSetHandle SenderLoop::Tasks::CreateEndpointSet::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_set_);
    return (EndpointSetHandle)endpoint_set_;
}

SenderLoop::Tasks::CreateEndpoint::CreateEndpoint(EndpointSetHandle endpoint_set,
                                                  address::Interface iface,
                                                  address::Protocol proto) {
    func_ = &SenderLoop::task_create_endpoint_;
    if (!endpoint_set) {
        roc_panic("sender sink: endpoint set handle is null");
    }
    endpoint_set_ = (SenderEndpointSet*)endpoint_set;
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

SenderLoop::Tasks::CheckEndpointSetIsReady::CheckEndpointSetIsReady(
    EndpointSetHandle endpoint_set) {
    func_ = &SenderLoop::task_check_endpoint_set_is_ready_;
    if (!endpoint_set) {
        roc_panic("sender sink: endpoint set handle is null");
    }
    endpoint_set_ = (SenderEndpointSet*)endpoint_set;
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

size_t SenderLoop::sample_rate() const {
    roc_panic_if_not(valid());

    return sink_.sample_rate();
}

size_t SenderLoop::num_channels() const {
    roc_panic_if_not(valid());

    return sink_.num_channels();
}

size_t SenderLoop::latency() const {
    roc_panic_if_not(valid());

    return sink_.latency();
}

bool SenderLoop::has_clock() const {
    roc_panic_if_not(valid());

    return sink_.has_clock();
}

void SenderLoop::write(audio::Frame& frame) {
    roc_panic_if_not(valid());

    core::Mutex::Lock lock(write_mutex_);

    if (ticker_) {
        ticker_->wait(timestamp_);
    }

    // Invokes process_subframe_imp() and process_task_imp().
    if (!process_subframes_and_tasks(frame)) {
        return;
    }

    timestamp_ += frame.size() / sink_.num_channels();
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

bool SenderLoop::task_create_endpoint_set_(Task& task) {
    task.endpoint_set_ = sink_.create_endpoint_set();
    return (bool)task.endpoint_set_;
}

bool SenderLoop::task_create_endpoint_(Task& task) {
    roc_panic_if(!task.endpoint_set_);

    task.endpoint_ = task.endpoint_set_->create_endpoint(task.iface_, task.proto_);
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

bool SenderLoop::task_check_endpoint_set_is_ready_(Task& task) {
    roc_panic_if(!task.endpoint_set_);

    return task.endpoint_set_->is_ready();
}

} // namespace pipeline
} // namespace roc
