/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/transcoder_sink.h"
#include "roc_audio/resampler_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

TranscoderSink::TranscoderSink(const TranscoderConfig& config,
                               audio::IFrameWriter* output_writer,
                               core::BufferFactory<audio::sample_t>& buffer_factory,
                               core::IArena& arena)
    : frame_writer_(NULL)
    , config_(config)
    , valid_(false) {
    config_.deduce_defaults();

    audio::IFrameWriter* awriter = output_writer;
    if (!awriter) {
        awriter = &null_writer_;
    }

    if (config_.input_sample_spec.channel_set()
        != config_.output_sample_spec.channel_set()) {
        channel_mapper_writer_.reset(
            new (channel_mapper_writer_) audio::ChannelMapperWriter(
                *awriter, buffer_factory,
                audio::SampleSpec(config_.output_sample_spec.sample_rate(),
                                  config_.input_sample_spec.pcm_format(),
                                  config_.input_sample_spec.channel_set()),
                config_.output_sample_spec));
        if (!channel_mapper_writer_ || !channel_mapper_writer_->is_valid()) {
            return;
        }
        awriter = channel_mapper_writer_.get();
    }

    if (config_.input_sample_spec.sample_rate()
        != config_.output_sample_spec.sample_rate()) {
        resampler_.reset(audio::ResamplerMap::instance().new_resampler(
            arena, buffer_factory, config_.resampler, config_.input_sample_spec,
            config_.output_sample_spec));

        if (!resampler_) {
            return;
        }

        resampler_writer_.reset(new (resampler_writer_) audio::ResamplerWriter(
            *awriter, *resampler_, buffer_factory, config_.input_sample_spec,
            audio::SampleSpec(config_.output_sample_spec.sample_rate(),
                              config_.input_sample_spec.pcm_format(),
                              config_.input_sample_spec.channel_set())));

        if (!resampler_writer_ || !resampler_writer_->is_valid()) {
            return;
        }
        awriter = resampler_writer_.get();
    }

    if (config_.enable_profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingWriter(
            *awriter, arena, config_.input_sample_spec, config_.profiler));
        if (!profiler_ || !profiler_->is_valid()) {
            return;
        }
        awriter = profiler_.get();
    }

    if (!awriter) {
        return;
    }

    frame_writer_ = awriter;
    valid_ = true;
}

bool TranscoderSink::is_valid() {
    return valid_;
}

sndio::ISink* TranscoderSink::to_sink() {
    return this;
}

sndio::ISource* TranscoderSink::to_source() {
    return NULL;
}

sndio::DeviceType TranscoderSink::type() const {
    return sndio::DeviceType_Sink;
}

sndio::DeviceState TranscoderSink::state() const {
    return sndio::DeviceState_Active;
}

void TranscoderSink::pause() {
    // no-op
}

bool TranscoderSink::resume() {
    return true;
}

bool TranscoderSink::restart() {
    return true;
}

audio::SampleSpec TranscoderSink::sample_spec() const {
    return config_.output_sample_spec;
}

core::nanoseconds_t TranscoderSink::latency() const {
    return 0;
}

bool TranscoderSink::has_latency() const {
    return false;
}

bool TranscoderSink::has_clock() const {
    return false;
}

void TranscoderSink::write(audio::Frame& frame) {
    roc_panic_if(!is_valid());

    frame_writer_->write(frame);
}

} // namespace pipeline
} // namespace roc
