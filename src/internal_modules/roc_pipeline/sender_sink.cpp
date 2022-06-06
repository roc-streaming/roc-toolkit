/*
 * Copyright (c) 2017 Roc Streaming authors
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
                       packet::PacketFactory& packet_factory,
                       core::BufferFactory<uint8_t>& byte_buffer_factory,
                       core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                       core::IAllocator& allocator)
    : config_(config)
    , format_map_(format_map)
    , packet_factory_(packet_factory)
    , byte_buffer_factory_(byte_buffer_factory)
    , sample_buffer_factory_(sample_buffer_factory)
    , allocator_(allocator)
    , audio_writer_(NULL)
    , num_channels_(config_.input_sample_spec.num_channels()) {
    audio::IWriter* awriter = &fanout_;

    if (config_.poisoning) {
        pipeline_poisoner_.reset(new (pipeline_poisoner_) audio::PoisonWriter(*awriter));
        if (!pipeline_poisoner_) {
            return;
        }
        awriter = pipeline_poisoner_.get();
    }

    if (config.profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingWriter(
            *awriter, allocator, config.input_sample_spec, config.profiler_config));
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

SenderEndpointSet* SenderSink::create_endpoint_set() {
    roc_panic_if(!valid());

    roc_log(LogInfo, "sender sink: adding endpoint set");

    core::SharedPtr<SenderEndpointSet> endpoint_set = new (allocator_)
        SenderEndpointSet(config_, format_map_, fanout_, packet_factory_,
                          byte_buffer_factory_, sample_buffer_factory_, allocator_);

    if (!endpoint_set) {
        roc_log(LogError, "sender sink: can't allocate endpoint set");
        return NULL;
    }

    endpoint_sets_.push_back(*endpoint_set);
    return endpoint_set.get();
}

size_t SenderSink::sample_rate() const {
    return config_.input_sample_spec.sample_rate();
}

size_t SenderSink::num_channels() const {
    return num_channels_;
}

bool SenderSink::has_clock() const {
    return config_.timing;
}

void SenderSink::write(audio::Frame& frame) {
    roc_panic_if(!valid());

    audio_writer_->write(frame);
}

} // namespace pipeline
} // namespace roc
