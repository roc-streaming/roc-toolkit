/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/sender_sink.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

SenderSink::Task::Task()
    : func_(NULL)
    , endpoint_set_(NULL)
    , endpoint_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , writer_(NULL) {
}

SenderSink::Tasks::AddEndpointSet::AddEndpointSet() {
    func_ = &SenderSink::task_add_endpoint_set_;
}

SenderSink::EndpointSetHandle SenderSink::Tasks::AddEndpointSet::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_set_);
    return (EndpointSetHandle)endpoint_set_;
}

SenderSink::Tasks::CreateEndpoint::CreateEndpoint(EndpointSetHandle endpoint_set,
                                                  address::Interface iface,
                                                  address::Protocol proto) {
    func_ = &SenderSink::task_create_endpoint_;
    if (!endpoint_set) {
        roc_panic("sender sink: endpoint set handle is null");
    }
    endpoint_set_ = (SenderEndpointSet*)endpoint_set;
    iface_ = iface;
    proto_ = proto;
}

SenderSink::EndpointHandle SenderSink::Tasks::CreateEndpoint::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_);
    return (EndpointHandle)endpoint_;
}

SenderSink::Tasks::SetEndpointOutputWriter::SetEndpointOutputWriter(
    EndpointHandle endpoint, packet::IWriter& writer) {
    func_ = &SenderSink::task_set_endpoint_output_writer_;
    if (!endpoint) {
        roc_panic("sender sink: endpoint handle is null");
    }
    endpoint_ = (SenderEndpoint*)endpoint;
    writer_ = &writer;
}

SenderSink::Tasks::SetEndpointDestinationUdpAddress::SetEndpointDestinationUdpAddress(
    EndpointHandle endpoint, const address::SocketAddr& addr) {
    func_ = &SenderSink::task_set_endpoint_destination_udp_address_;
    if (!endpoint) {
        roc_panic("sender sink: endpoint handle is null");
    }
    endpoint_ = (SenderEndpoint*)endpoint;
    addr_ = addr;
}

SenderSink::Tasks::CheckEndpointSetIsReady::CheckEndpointSetIsReady(
    EndpointSetHandle endpoint_set) {
    func_ = &SenderSink::task_check_endpoint_set_is_ready_;
    if (!endpoint_set) {
        roc_panic("sender sink: endpoint set handle is null");
    }
    endpoint_set_ = (SenderEndpointSet*)endpoint_set;
}

SenderSink::SenderSink(ITaskScheduler& scheduler,
                       const SenderConfig& config,
                       const rtp::FormatMap& format_map,
                       packet::PacketPool& packet_pool,
                       core::BufferPool<uint8_t>& byte_buffer_pool,
                       core::BufferPool<audio::sample_t>& sample_buffer_pool,
                       core::IAllocator& allocator)
    : TaskPipeline(
        scheduler, config.tasks, config.input_sample_rate, config.input_channels)
    , config_(config)
    , format_map_(format_map)
    , packet_pool_(packet_pool)
    , byte_buffer_pool_(byte_buffer_pool)
    , sample_buffer_pool_(sample_buffer_pool)
    , allocator_(allocator)
    , audio_writer_(NULL)
    , timestamp_(0)
    , num_channels_(packet::num_channels(config_.input_channels)) {
    if (config_.timing) {
        ticker_.reset(new (allocator_) core::Ticker(config_.input_sample_rate),
                      allocator_);
        if (!ticker_) {
            return;
        }
    }

    audio::IWriter* awriter = &fanout_;

    if (config_.poisoning) {
        pipeline_poisoner_.reset(new (allocator) audio::PoisonWriter(*awriter),
                                 allocator);
        if (!pipeline_poisoner_) {
            return;
        }
        awriter = pipeline_poisoner_.get();
    }

    if (config.profiling) {
        profiler_.reset(new (allocator) audio::ProfilingWriter(
                            *awriter, allocator, config.input_channels,
                            config.input_sample_rate, config.profiler_config),
                        allocator);
        if (!profiler_ || !profiler_->valid()) {
            return;
        }
        awriter = profiler_.get();
    }

    audio_writer_ = awriter;
}

bool SenderSink::valid() const {
    return audio_writer_;
}

size_t SenderSink::sample_rate() const {
    return config_.input_sample_rate;
}

size_t SenderSink::num_channels() const {
    return num_channels_;
}

bool SenderSink::has_clock() const {
    return config_.timing;
}

void SenderSink::write(audio::Frame& frame) {
    core::Mutex::Lock lock(write_mutex_);

    roc_panic_if(!valid());

    if (ticker_) {
        ticker_->wait(timestamp_);
    }

    process_frame_and_tasks(frame);
}

core::nanoseconds_t SenderSink::timestamp_imp() const {
    return core::timestamp();
}

bool SenderSink::process_frame_imp(audio::Frame& frame) {
    audio_writer_->write(frame);

    timestamp_ += frame.size() / num_channels_;

    return true;
}

bool SenderSink::process_task_imp(TaskPipeline::Task& basic_task) {
    Task& task = (Task&)basic_task;

    roc_panic_if_not(task.func_);

    return (this->*(task.func_))(task);
}

bool SenderSink::task_add_endpoint_set_(Task& task) {
    roc_log(LogInfo, "sender sink: adding endpoint set");

    core::SharedPtr<SenderEndpointSet> endpoint_set = new (allocator_)
        SenderEndpointSet(config_, format_map_, packet_pool_, byte_buffer_pool_,
                          sample_buffer_pool_, allocator_);

    if (!endpoint_set) {
        roc_log(LogError, "sender sink: can't allocate endpoint set");
        return false;
    }

    endpoint_sets_.push_back(*endpoint_set);

    task.endpoint_set_ = endpoint_set.get();
    return true;
}

bool SenderSink::task_create_endpoint_(Task& task) {
    roc_panic_if(!task.endpoint_set_);

    SenderEndpoint* endpoint =
        task.endpoint_set_->create_endpoint(task.iface_, task.proto_);
    if (!endpoint) {
        return 0;
    }

    if (audio::IWriter* endpoint_set_writer = task.endpoint_set_->writer()) {
        if (!fanout_.has_output(*endpoint_set_writer)) {
            fanout_.add_output(*endpoint_set_writer);
        }
    }

    task.endpoint_ = endpoint;
    return true;
}

bool SenderSink::task_set_endpoint_output_writer_(Task& task) {
    roc_panic_if(!task.endpoint_);
    roc_panic_if(!task.writer_);

    task.endpoint_->set_output_writer(*task.writer_);
    return true;
}

bool SenderSink::task_set_endpoint_destination_udp_address_(Task& task) {
    roc_panic_if(!task.endpoint_);

    task.endpoint_->set_destination_udp_address(task.addr_);
    return true;
}

bool SenderSink::task_check_endpoint_set_is_ready_(Task& task) {
    roc_panic_if(!task.endpoint_set_);

    return task.endpoint_set_->is_ready();
}

} // namespace pipeline
} // namespace roc
