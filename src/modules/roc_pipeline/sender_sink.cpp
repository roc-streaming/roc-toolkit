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

SenderSink::EndpointSetHandle SenderSink::add_endpoint_set() {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    roc_log(LogInfo, "sender sink: adding endpoint set");

    core::SharedPtr<SenderEndpointSet> endpoint_set = new (allocator_)
        SenderEndpointSet(config_, format_map_, packet_pool_, byte_buffer_pool_,
                          sample_buffer_pool_, allocator_);

    if (!endpoint_set) {
        roc_log(LogError, "sender sink: can't allocate endpoint set");
        return 0;
    }

    endpoint_sets_.push_back(*endpoint_set);

    return (EndpointSetHandle)endpoint_set.get();
}

SenderSink::EndpointHandle SenderSink::add_endpoint(EndpointSetHandle endpoint_set_handle,
                                                    address::EndpointType type,
                                                    address::EndpointProtocol proto) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    SenderEndpointSet* endpoint_set = (SenderEndpointSet*)endpoint_set_handle;
    roc_panic_if_not(endpoint_set);

    SenderEndpoint* endpoint = endpoint_set->add_endpoint(type, proto);
    if (!endpoint) {
        return 0;
    }

    if (audio::IWriter* endpoint_set_writer = endpoint_set->writer()) {
        if (!fanout_.has_output(*endpoint_set_writer)) {
            fanout_.add_output(*endpoint_set_writer);
        }
    }

    return (EndpointHandle)endpoint;
}

void SenderSink::set_endpoint_output_writer(EndpointHandle endpoint_handle,
                                            packet::IWriter& writer) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    SenderEndpoint* endpoint = (SenderEndpoint*)endpoint_handle;
    roc_panic_if_not(endpoint);

    endpoint->set_output_writer(writer);
}

void SenderSink::set_endpoint_destination_udp_address(EndpointHandle endpoint_handle,
                                                      const address::SocketAddr& addr) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    SenderEndpoint* endpoint = (SenderEndpoint*)endpoint_handle;
    roc_panic_if_not(endpoint);

    endpoint->set_destination_udp_address(addr);
}

bool SenderSink::is_endpoint_set_ready(EndpointSetHandle endpoint_set_handle) const {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if(!valid());

    SenderEndpointSet* endpoint_set = (SenderEndpointSet*)endpoint_set_handle;
    roc_panic_if_not(endpoint_set);

    return endpoint_set->is_ready();
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
