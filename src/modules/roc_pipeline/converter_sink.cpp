/*
 * Copyright (c) 2019 Roc authors
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
                             core::BufferPool<audio::sample_t>& pool,
                             core::IAllocator& allocator)
    : audio_writer_(NULL)
    , config_(config)
    , num_channels_(packet::num_channels(config.output_channels)) {
    audio::IWriter* awriter = output_writer;
    if (!awriter) {
        awriter = &null_writer_;
    }

    if (config.resampling) {
        if (config.poisoning) {
            resampler_poisoner_.reset(new (allocator) audio::PoisonWriter(*awriter),
                                      allocator);
            if (!resampler_poisoner_) {
                return;
            }
            awriter = resampler_poisoner_.get();
        }

        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
                             config.resampler_backend, allocator,
                             config.resampler_profile, config.internal_frame_length,
                             config.input_sample_rate, config.input_channels),
                         allocator);

        if (!resampler_) {
            return;
        }

        resampler_writer_.reset(new (allocator) audio::ResamplerWriter(
                                    *awriter, *resampler_, pool,
                                    config.internal_frame_length,
                                    config.input_sample_rate, config.input_channels),
                                allocator);

        if (!resampler_writer_ || !resampler_writer_->valid()) {
            return;
        }
        if (!resampler_writer_->set_scaling(config.input_sample_rate,
                                            config.output_sample_rate, 1.0f)) {
            return;
        }
        awriter = resampler_writer_.get();
    }

    if (config.poisoning) {
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

bool ConverterSink::valid() {
    return audio_writer_;
}

size_t ConverterSink::sample_rate() const {
    return config_.output_sample_rate;
}

size_t ConverterSink::num_channels() const {
    return num_channels_;
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
