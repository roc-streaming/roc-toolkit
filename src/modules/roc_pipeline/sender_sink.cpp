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
#include "roc_pipeline/port_to_str.h"

namespace roc {
namespace pipeline {

SenderSink::SenderSink(const SenderConfig& config,
                       const rtp::FormatMap& format_map,
                       packet::PacketPool& packet_pool,
                       core::BufferPool<uint8_t>& byte_buffer_pool,
                       core::BufferPool<audio::sample_t>& sample_buffer_pool,
                       core::IAllocator& allocator)
    : config_(config)
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

    audio_writer_ = awriter;
}

bool SenderSink::valid() const {
    return audio_writer_;
}

SenderSink::PortGroupID SenderSink::add_port_group() {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    roc_log(LogInfo, "sender sink: adding port group");

    core::SharedPtr<SenderPortGroup> port_group = new (allocator_)
        SenderPortGroup(config_, format_map_, packet_pool_, byte_buffer_pool_,
                        sample_buffer_pool_, allocator_);

    if (!port_group) {
        roc_log(LogError, "sender sink: can't allocate port group");
        return 0;
    }

    port_groups_.push_back(*port_group);

    return (PortGroupID)port_group.get();
}

SenderSink::PortID SenderSink::add_port(PortGroupID port_group_id,
                                        address::EndpointType port_type,
                                        const pipeline::PortConfig& port_config) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    roc_log(LogInfo, "sender sink: adding port");

    SenderPortGroup* port_group = (SenderPortGroup*)port_group_id;
    roc_panic_if_not(port_group);

    SenderPort* port = port_group->add_port(port_type, port_config);
    if (!port) {
        return 0;
    }

    if (audio::IWriter* port_group_writer = port_group->writer()) {
        if (!fanout_.has_output(*port_group_writer)) {
            fanout_.add_output(*port_group_writer);
        }
    }

    return (PortID)port;
}

void SenderSink::set_port_writer(PortID port_id, packet::IWriter& port_writer) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    roc_log(LogDebug, "sender sink: attaching writer to port");

    SenderPort* port = (SenderPort*)port_id;
    roc_panic_if_not(port);

    port->set_writer(port_writer);
}

bool SenderSink::port_group_configured(PortGroupID port_group_id) const {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    SenderPortGroup* port_group = (SenderPortGroup*)port_group_id;
    roc_panic_if_not(port_group);

    return port_group->is_configured();
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
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    if (ticker_) {
        ticker_->wait(timestamp_);
    }

    audio_writer_->write(frame);

    timestamp_ += frame.size() / num_channels_;
}

} // namespace pipeline
} // namespace roc
