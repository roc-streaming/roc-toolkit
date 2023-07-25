/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/converter_sink.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

ConverterSink::ConverterSink(const ConverterConfig& config,
                             audio::IFrameWriter* output_writer,
                             core::BufferFactory<audio::sample_t>& buffer_factory,
                             core::IAllocator& allocator)
    : audio_writer_(NULL)
    , config_(config) {
    audio::IFrameWriter* awriter = output_writer;
    if (!awriter) {
        awriter = &null_writer_;
    }

    if (config.input_sample_spec.channel_set()
        != config.output_sample_spec.channel_set()) {
        channel_mapper_writer_.reset(
            new (channel_mapper_writer_) audio::ChannelMapperWriter(
                *awriter, buffer_factory, config.internal_frame_length,
                audio::SampleSpec(config.output_sample_spec.sample_rate(),
                                  config.input_sample_spec.channel_set()),
                config.output_sample_spec));
        if (!channel_mapper_writer_ || !channel_mapper_writer_->is_valid()) {
            return;
        }
        awriter = channel_mapper_writer_.get();
    }

    if (config.input_sample_spec.sample_rate()
        != config.output_sample_spec.sample_rate()) {
        if (config.enable_poisoning) {
            resampler_poisoner_.reset(new (resampler_poisoner_)
                                          audio::PoisonWriter(*awriter));
            if (!resampler_poisoner_) {
                return;
            }
            awriter = resampler_poisoner_.get();
        }

        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
                             config.resampler_backend, allocator, buffer_factory,
                             config.resampler_profile, config.internal_frame_length,
                             config.input_sample_spec),
                         allocator);

        if (!resampler_) {
            return;
        }

        resampler_writer_.reset(new (resampler_writer_) audio::ResamplerWriter(
            *awriter, *resampler_, buffer_factory, config.internal_frame_length,
            config.input_sample_spec,
            audio::SampleSpec(config.output_sample_spec.sample_rate(),
                              config.input_sample_spec.channel_set())));

        if (!resampler_writer_ || !resampler_writer_->is_valid()) {
            return;
        }
        awriter = resampler_writer_.get();
    }

    if (config.enable_poisoning) {
        pipeline_poisoner_.reset(new (pipeline_poisoner_) audio::PoisonWriter(*awriter));
        if (!pipeline_poisoner_) {
            return;
        }
        awriter = pipeline_poisoner_.get();
    }

    if (config.enable_profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingWriter(
            *awriter, allocator, config.input_sample_spec, config.profiler_config));
        if (!profiler_ || !profiler_->is_valid()) {
            return;
        }
        awriter = profiler_.get();
    }

    audio_writer_ = awriter;
}

bool ConverterSink::is_valid() {
    return audio_writer_;
}

sndio::DeviceType ConverterSink::type() const {
    return sndio::DeviceType_Sink;
}

sndio::DeviceState ConverterSink::state() const {
    return sndio::DeviceState_Active;
}

void ConverterSink::pause() {
    // no-op
}

bool ConverterSink::resume() {
    return true;
}

bool ConverterSink::restart() {
    return true;
}

audio::SampleSpec ConverterSink::sample_spec() const {
    return config_.output_sample_spec;
}

core::nanoseconds_t ConverterSink::latency() const {
    return 0;
}

bool ConverterSink::has_clock() const {
    return false;
}

void ConverterSink::write(audio::Frame& frame) {
    roc_panic_if(!is_valid());

    audio_writer_->write(frame);
}

} // namespace pipeline
} // namespace roc
