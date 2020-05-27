/*
 * Copyright (c) 2020 Roc authors
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
                                 core::BufferPool<audio::sample_t>& pool,
                                 core::IAllocator& allocator)
    : input_source_(input_source)
    , audio_reader_(NULL)
    , config_(config) {
    audio::IReader* areader = &input_source_;

    if (config.resampling && config.output_sample_rate != config.input_sample_rate) {
        if (config.poisoning) {
            resampler_poisoner_.reset(new (allocator) audio::PoisonReader(*areader),
                                      allocator);
            if (!resampler_poisoner_) {
                return;
            }
            areader = resampler_poisoner_.get();
        }

        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
                             config.resampler_backend, allocator, pool,
                             config.resampler_profile, config.internal_frame_length,
                             config.input_sample_rate, config.input_channels),
                         allocator);

        if (!resampler_) {
            return;
        }

        resampler_reader_.reset(
            new (allocator) audio::ResamplerReader(*areader, *resampler_), allocator);

        if (!resampler_reader_ || !resampler_reader_->valid()) {
            return;
        }
        if (!resampler_reader_->set_scaling(config.input_sample_rate,
                                            config.output_sample_rate, 1.0f)) {
            return;
        }
        areader = resampler_reader_.get();
    }

    if (config.poisoning) {
        pipeline_poisoner_.reset(new (allocator) audio::PoisonReader(*areader),
                                 allocator);
        if (!pipeline_poisoner_) {
            return;
        }
        areader = pipeline_poisoner_.get();
    }

    if (config.profiling) {
        profiler_.reset(new (allocator) audio::ProfilingReader(
                            *areader, allocator, config.output_channels,
                            config.output_sample_rate, config.profiler_config),
                        allocator);
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
    return config_.output_sample_rate;
}

size_t ConverterSource::num_channels() const {
    return input_source_.num_channels();
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

bool ConverterSource::read(audio::Frame& frame) {
    roc_panic_if(!valid());

    return audio_reader_->read(frame);
}

} // namespace pipeline
} // namespace roc
