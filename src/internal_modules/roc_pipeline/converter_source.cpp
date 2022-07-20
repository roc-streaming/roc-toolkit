/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/converter_source.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

ConverterSource::ConverterSource(const ConverterConfig& config,
                                 sndio::ISource& input_source,
                                 core::BufferFactory<audio::sample_t>& buffer_factory,
                                 core::IAllocator& allocator)
    : input_source_(input_source)
    , audio_reader_(NULL)
    , config_(config) {
    audio::IReader* areader = &input_source_;

    if (config.input_sample_spec.channel_mask()
        != config.output_sample_spec.channel_mask()) {
        channel_mapper_reader_.reset(
            new (channel_mapper_reader_) audio::ChannelMapperReader(
                *areader, buffer_factory, config.internal_frame_length,
                config.input_sample_spec,
                audio::SampleSpec(config.input_sample_spec.sample_rate(),
                                  config.output_sample_spec.channel_mask())));
        if (!channel_mapper_reader_ || !channel_mapper_reader_->valid()) {
            return;
        }
        areader = channel_mapper_reader_.get();
    }

    if (config.resampling
        && config.input_sample_spec.sample_rate()
            != config.output_sample_spec.sample_rate()) {
        if (config.poisoning) {
            resampler_poisoner_.reset(new (resampler_poisoner_)
                                          audio::PoisonReader(*areader));
            if (!resampler_poisoner_) {
                return;
            }
            areader = resampler_poisoner_.get();
        }

        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
                             config.resampler_backend, allocator, buffer_factory,
                             config.resampler_profile, config.internal_frame_length,
                             audio::SampleSpec(config.input_sample_spec.sample_rate(),
                                               config.output_sample_spec.channel_mask())),
                         allocator);

        if (!resampler_) {
            return;
        }

        resampler_reader_.reset(new (resampler_reader_) audio::ResamplerReader(
            *areader, *resampler_,
            audio::SampleSpec(config.input_sample_spec.sample_rate(),
                              config.output_sample_spec.channel_mask()),
            config.output_sample_spec));

        if (!resampler_reader_ || !resampler_reader_->valid()) {
            return;
        }
        areader = resampler_reader_.get();
    }

    if (config.poisoning) {
        pipeline_poisoner_.reset(new (pipeline_poisoner_) audio::PoisonReader(*areader));
        if (!pipeline_poisoner_) {
            return;
        }
        areader = pipeline_poisoner_.get();
    }

    if (config.profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingReader(
            *areader, allocator, config.output_sample_spec, config.profiler_config));
        if (!profiler_ || !profiler_->valid()) {
            return;
        }
        areader = profiler_.get();
    }

    audio_reader_ = areader;
}

bool ConverterSource::valid() {
    return audio_reader_;
}

size_t ConverterSource::sample_rate() const {
    return config_.output_sample_spec.sample_rate();
}

size_t ConverterSource::num_channels() const {
    return input_source_.num_channels();
}

size_t ConverterSource::latency() const {
    return 0;
}

bool ConverterSource::has_clock() const {
    return input_source_.has_clock();
}

sndio::ISource::State ConverterSource::state() const {
    return input_source_.state();
}

void ConverterSource::pause() {
    input_source_.pause();
}

bool ConverterSource::resume() {
    return input_source_.resume();
}

bool ConverterSource::restart() {
    return input_source_.restart();
}

void ConverterSource::reclock(packet::ntp_timestamp_t timestamp) {
    input_source_.reclock(timestamp);
}

bool ConverterSource::read(audio::Frame& frame) {
    roc_panic_if(!valid());

    return audio_reader_->read(frame);
}

} // namespace pipeline
} // namespace roc
