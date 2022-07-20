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
                             audio::IWriter* output_writer,
                             core::BufferFactory<audio::sample_t>& buffer_factory,
                             core::IAllocator& allocator)
    : audio_writer_(NULL)
    , config_(config)
    , num_channels_(config.output_sample_spec.num_channels()) {
    audio::IWriter* awriter = output_writer;
    if (!awriter) {
        awriter = &null_writer_;
    }

    if (config.input_sample_spec.channel_mask()
        != config.output_sample_spec.channel_mask()) {
        channel_mapper_writer_.reset(
            new (channel_mapper_writer_) audio::ChannelMapperWriter(
                *awriter, buffer_factory, config.internal_frame_length,
                audio::SampleSpec(config.output_sample_spec.sample_rate(),
                                  config.input_sample_spec.channel_mask()),
                config.output_sample_spec));
        if (!channel_mapper_writer_ || !channel_mapper_writer_->valid()) {
            return;
        }
        awriter = channel_mapper_writer_.get();
    }

    if (config.resampling
        && config.input_sample_spec.sample_rate()
            != config.output_sample_spec.sample_rate()) {
        if (config.poisoning) {
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
                              config.input_sample_spec.channel_mask())));

        if (!resampler_writer_ || !resampler_writer_->valid()) {
            return;
        }
        awriter = resampler_writer_.get();
    }

    if (config.poisoning) {
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

bool ConverterSink::valid() {
    return audio_writer_;
}

size_t ConverterSink::sample_rate() const {
    return config_.output_sample_spec.sample_rate();
}

size_t ConverterSink::num_channels() const {
    return num_channels_;
}

size_t ConverterSink::latency() const {
    return 0;
}

bool ConverterSink::has_clock() const {
    return false;
}

void ConverterSink::write(audio::Frame& frame) {
    roc_panic_if(!valid());

    audio_writer_->write(frame);
}

} // namespace pipeline
} // namespace roc
