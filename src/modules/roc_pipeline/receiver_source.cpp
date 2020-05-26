/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_source.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/shared_ptr.h"

namespace roc {
namespace pipeline {

ReceiverSource::Task::Task()
    : func_(NULL)
    , endpoint_set_(NULL)
    , iface_(address::Iface_Invalid)
    , proto_(address::Proto_None)
    , writer_(NULL) {
}

ReceiverSource::Tasks::AddEndpointSet::AddEndpointSet() {
    func_ = &ReceiverSource::task_add_endpoint_set_;
}

ReceiverSource::EndpointSetHandle
ReceiverSource::Tasks::AddEndpointSet::get_handle() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(endpoint_set_);
    return (EndpointSetHandle)endpoint_set_;
}

ReceiverSource::Tasks::CreateEndpoint::CreateEndpoint(EndpointSetHandle endpoint_set,
                                                      address::Interface iface,
                                                      address::Protocol proto) {
    func_ = &ReceiverSource::task_create_endpoint_;
    if (!endpoint_set) {
        roc_panic("receiver source: endpoint set handle is null");
    }
    endpoint_set_ = (ReceiverEndpointSet*)endpoint_set;
    iface_ = iface;
    proto_ = proto;
}

packet::IWriter* ReceiverSource::Tasks::CreateEndpoint::get_writer() const {
    if (!success()) {
        return NULL;
    }
    roc_panic_if_not(writer_);
    return writer_;
}

ReceiverSource::Tasks::DeleteEndpoint::DeleteEndpoint(EndpointSetHandle endpoint_set,
                                                      address::Interface iface) {
    func_ = &ReceiverSource::task_delete_endpoint_;
    if (!endpoint_set) {
        roc_panic("receiver source: endpoint set handle is null");
    }
    endpoint_set_ = (ReceiverEndpointSet*)endpoint_set;
    iface_ = iface;
}

ReceiverSource::ReceiverSource(ITaskScheduler& scheduler,
                               const ReceiverConfig& config,
                               const rtp::FormatMap& format_map,
                               packet::PacketPool& packet_pool,
                               core::BufferPool<uint8_t>& byte_buffer_pool,
                               core::BufferPool<audio::sample_t>& sample_buffer_pool,
                               core::IAllocator& allocator)
    : TaskPipeline(scheduler,
                   config.tasks,
                   config.common.output_sample_rate,
                   config.common.output_channels)
    , format_map_(format_map)
    , packet_pool_(packet_pool)
    , byte_buffer_pool_(byte_buffer_pool)
    , sample_buffer_pool_(sample_buffer_pool)
    , allocator_(allocator)
    , ticker_(config.common.output_sample_rate)
    , audio_reader_(NULL)
    , config_(config)
    , timestamp_(0)
    , num_channels_(packet::num_channels(config.common.output_channels)) {
    mixer_.reset(new (allocator_) audio::Mixer(
                     sample_buffer_pool, config.common.internal_frame_length,
                     config.common.output_sample_rate, config.common.output_channels),
                 allocator_);
    if (!mixer_ || !mixer_->valid()) {
        return;
    }
    audio::IReader* areader = mixer_.get();

    if (config.common.poisoning) {
        poisoner_.reset(new (allocator_) audio::PoisonReader(*areader), allocator_);
        if (!poisoner_) {
            return;
        }
        areader = poisoner_.get();
    }

    if (config.common.profiling) {
        profiler_.reset(new (allocator) audio::ProfilingReader(
                            *areader, allocator, config.common.output_channels,
                            config.common.output_sample_rate,
                            config.common.profiler_config),
                        allocator);
        if (!profiler_ || !profiler_->valid()) {
            return;
        }
        areader = profiler_.get();
    }

    audio_reader_ = areader;
}

bool ReceiverSource::valid() const {
    return audio_reader_;
}

bool ReceiverSource::read(audio::Frame& frame) {
    roc_panic_if(!valid());

    core::Mutex::Lock lock(read_mutex_);

    if (config_.common.timing) {
        ticker_.wait(timestamp_);
    }

    return process_frame_and_tasks(frame);
}

size_t ReceiverSource::num_sessions() const {
    return receiver_state_.num_sessions();
}

size_t ReceiverSource::sample_rate() const {
    return config_.common.output_sample_rate;
}

size_t ReceiverSource::num_channels() const {
    return num_channels_;
}

bool ReceiverSource::has_clock() const {
    return config_.common.timing;
}

sndio::ISource::State ReceiverSource::state() const {
    roc_panic_if(!valid());

    if (receiver_state_.num_sessions() != 0) {
        // we have sessions and they're producing some sound
        return Playing;
    }

    if (receiver_state_.has_pending_packets()) {
        // we don't have sessions, but we have packets that may create sessions
        return Playing;
    }

    // no sessions and packets; we can sleep until there are some
    return Idle;
}

void ReceiverSource::pause() {
    // no-op
}

bool ReceiverSource::resume() {
    return true;
}

bool ReceiverSource::restart() {
    return true;
}

core::nanoseconds_t ReceiverSource::timestamp_imp() const {
    return core::timestamp();
}

bool ReceiverSource::process_frame_imp(audio::Frame& frame) {
    for (core::SharedPtr<ReceiverEndpointSet> endpoint_set = endpoint_sets_.front();
         endpoint_set; endpoint_set = endpoint_sets_.nextof(*endpoint_set)) {
        endpoint_set->update(timestamp_);
    }

    if (!audio_reader_->read(frame)) {
        return false;
    }

    timestamp_ += frame.size() / num_channels_;

    return true;
}

bool ReceiverSource::process_task_imp(TaskPipeline::Task& basic_task) {
    Task& task = (Task&)basic_task;

    roc_panic_if_not(task.func_);

    return (this->*(task.func_))(task);
}

bool ReceiverSource::task_add_endpoint_set_(Task& task) {
    core::SharedPtr<ReceiverEndpointSet> endpoint_set = new (allocator_)
        ReceiverEndpointSet(config_, receiver_state_, *mixer_, format_map_, packet_pool_,
                            byte_buffer_pool_, sample_buffer_pool_, allocator_);
    if (!endpoint_set) {
        return false;
    }

    endpoint_sets_.push_back(*endpoint_set);
    task.endpoint_set_ = endpoint_set.get();

    return true;
}

bool ReceiverSource::task_create_endpoint_(Task& task) {
    packet::IWriter* writer =
        task.endpoint_set_->create_endpoint(task.iface_, task.proto_);
    if (!writer) {
        return false;
    }
    task.writer_ = writer;
    return true;
}

bool ReceiverSource::task_delete_endpoint_(Task& task) {
    task.endpoint_set_->delete_endpoint(task.iface_);
    return true;
}

} // namespace pipeline
} // namespace roc
