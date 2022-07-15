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
    , endpoint_set_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , writer_(NULL) {
}

ReceiverLoop::Tasks::CreateEndpointSet::CreateEndpointSet() {
    func_ = &ReceiverLoop::task_create_endpoint_set_;
}

ReceiverLoop::EndpointSetHandle
ReceiverLoop::Tasks::CreateEndpointSet::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_set_);
    return (EndpointSetHandle)endpoint_set_;
}

ReceiverLoop::Tasks::CreateEndpoint::CreateEndpoint(EndpointSetHandle endpoint_set,
                                                    address::Interface iface,
                                                    address::Protocol proto) {
    func_ = &ReceiverLoop::task_create_endpoint_;
    if (!endpoint_set) {
        roc_panic("receiver source: endpoint set handle is null");
    }
    endpoint_set_ = (ReceiverEndpointSet*)endpoint_set;
    iface_ = iface;
    proto_ = proto;
}

packet::IWriter* ReceiverLoop::Tasks::CreateEndpoint::get_writer() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(writer_);
    return writer_;
}

ReceiverLoop::Tasks::DeleteEndpoint::DeleteEndpoint(EndpointSetHandle endpoint_set,
                                                    address::Interface iface) {
    func_ = &ReceiverLoop::task_delete_endpoint_;
    if (!endpoint_set) {
        roc_panic("receiver source: endpoint set handle is null");
    }
    endpoint_set_ = (ReceiverEndpointSet*)endpoint_set;
    iface_ = iface;
}

ReceiverLoop::ReceiverLoop(IPipelineTaskScheduler& scheduler,
                           const ReceiverConfig& config,
                           const rtp::FormatMap& format_map,
                           packet::PacketFactory& packet_factory,
                           core::BufferFactory<uint8_t>& byte_buffer_factory,
                           core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                           core::IAllocator& allocator)
    : PipelineLoop(scheduler, config.tasks, config.common.output_sample_spec)
    , source_(config,
              format_map,
              packet_factory,
              byte_buffer_factory,
              sample_buffer_factory,
              allocator)
    , timestamp_(0)
    , valid_(false) {
    if (!source_.valid()) {
        return;
    }

    if (config.common.timing) {
        ticker_.reset(new (ticker_)
                          core::Ticker(config.common.output_sample_spec.sample_rate()));
        if (!ticker_) {
            return;
        }
    }

    valid_ = true;
}

bool ReceiverLoop::valid() const {
    return valid_;
}

sndio::ISource& ReceiverLoop::source() {
    roc_panic_if(!valid());

    return *this;
}

size_t ReceiverLoop::sample_rate() const {
    roc_panic_if(!valid());

    return source_.sample_rate();
}

size_t ReceiverLoop::num_channels() const {
    roc_panic_if(!valid());

    return source_.num_channels();
}

bool ReceiverLoop::has_clock() const {
    roc_panic_if(!valid());

    return source_.has_clock();
}

sndio::ISource::State ReceiverLoop::state() const {
    roc_panic_if(!valid());

    return source_.state();
}

void ReceiverLoop::pause() {
    roc_panic_if(!valid());

    source_.pause();
}

bool ReceiverLoop::resume() {
    roc_panic_if(!valid());

    return source_.resume();
}

bool ReceiverLoop::restart() {
    roc_panic_if(!valid());

    return source_.restart();
}

void ReceiverLoop::reclock(packet::ntp_timestamp_t timestamp) {
    roc_panic_if(!valid());

    core::Mutex::Lock lock(read_mutex_);

    source_.reclock(timestamp);
}

bool ReceiverLoop::read(audio::Frame& frame) {
    roc_panic_if(!valid());

    core::Mutex::Lock lock(read_mutex_);

    if (ticker_) {
        ticker_->wait(timestamp_);
    }

    // Invokes process_subframe_imp() and process_task_imp().
    if (!process_subframes_and_tasks(frame)) {
        return false;
    }

    timestamp_ += frame.size() / source_.num_channels();

    return true;
}

core::nanoseconds_t ReceiverLoop::timestamp_imp() const {
    return core::timestamp();
}

bool ReceiverLoop::process_subframe_imp(audio::Frame& frame) {
    return source_.read(frame);
}

bool ReceiverLoop::process_task_imp(PipelineTask& basic_task) {
    Task& task = (Task&)basic_task;

    roc_panic_if_not(task.func_);
    return (this->*(task.func_))(task);
}

bool ReceiverLoop::task_create_endpoint_set_(Task& task) {
    task.endpoint_set_ = source_.create_endpoint_set();
    return (bool)task.endpoint_set_;
}

bool ReceiverLoop::task_create_endpoint_(Task& task) {
    ReceiverEndpoint* endpoint =
        task.endpoint_set_->create_endpoint(task.iface_, task.proto_);
    if (!endpoint) {
        return false;
    }
    task.writer_ = &endpoint->writer();
    return true;
}

bool ReceiverLoop::task_delete_endpoint_(Task& task) {
    task.endpoint_set_->delete_endpoint(task.iface_);
    return true;
}

} // namespace pipeline
} // namespace roc
