/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/transcoder_sink.h"
#include "roc_audio/processor_map.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace pipeline {

TranscoderSink::TranscoderSink(const TranscoderConfig& config,
                               audio::IFrameWriter* output_writer,
                               audio::ProcessorMap& processor_map,
                               core::IPool& frame_pool,
                               core::IPool& frame_buffer_pool,
                               core::IArena& arena)
    : IDevice(arena)
    , ISink(arena)
    , frame_factory_(frame_pool, frame_buffer_pool)
    , frame_writer_(NULL)
    , config_(config)
    , init_status_(status::NoStatus) {
    if (!config_.deduce_defaults(processor_map)) {
        init_status_ = status::StatusBadConfig;
        return;
    }

    audio::IFrameWriter* frm_writer = output_writer;
    if (!frm_writer) {
        frm_writer = &null_writer_;
    }

    if (config_.input_sample_spec.channel_set()
        != config_.output_sample_spec.channel_set()) {
        const audio::SampleSpec from_spec(config_.output_sample_spec.sample_rate(),
                                          audio::PcmSubformat_Raw,
                                          config_.input_sample_spec.channel_set());

        const audio::SampleSpec to_spec(config_.output_sample_spec.sample_rate(),
                                        audio::PcmSubformat_Raw,
                                        config_.output_sample_spec.channel_set());

        channel_mapper_writer_.reset(
            new (channel_mapper_writer_) audio::ChannelMapperWriter(
                *frm_writer, frame_factory_, from_spec, to_spec));
        if ((init_status_ = channel_mapper_writer_->init_status()) != status::StatusOK) {
            return;
        }
        frm_writer = channel_mapper_writer_.get();
    }

    if (config_.input_sample_spec.sample_rate()
        != config_.output_sample_spec.sample_rate()) {
        const audio::SampleSpec from_spec(config_.input_sample_spec.sample_rate(),
                                          audio::PcmSubformat_Raw,
                                          config_.input_sample_spec.channel_set());

        const audio::SampleSpec to_spec(config_.output_sample_spec.sample_rate(),
                                        audio::PcmSubformat_Raw,
                                        config_.input_sample_spec.channel_set());

        resampler_.reset(processor_map.new_resampler(config_.resampler, from_spec,
                                                     to_spec, frame_factory_, arena));
        if (!resampler_) {
            init_status_ = status::StatusNoMem;
            return;
        }
        if ((init_status_ = resampler_->init_status()) != status::StatusOK) {
            return;
        }

        resampler_writer_.reset(new (resampler_writer_) audio::ResamplerWriter(
            *frm_writer, frame_factory_, *resampler_, from_spec, to_spec));
        if ((init_status_ = resampler_writer_->init_status()) != status::StatusOK) {
            return;
        }
        frm_writer = resampler_writer_.get();
    }

    if (config_.enable_profiling) {
        profiler_.reset(new (profiler_) audio::ProfilingWriter(
            *frm_writer, arena, config_.input_sample_spec, config_.profiler));
        if ((init_status_ = profiler_->init_status()) != status::StatusOK) {
            return;
        }
        frm_writer = profiler_.get();
    }

    frame_writer_ = frm_writer;
    init_status_ = status::StatusOK;
}

status::StatusCode TranscoderSink::init_status() const {
    return init_status_;
}

sndio::DeviceType TranscoderSink::type() const {
    return sndio::DeviceType_Sink;
}

sndio::ISink* TranscoderSink::to_sink() {
    return this;
}

sndio::ISource* TranscoderSink::to_source() {
    return NULL;
}

audio::SampleSpec TranscoderSink::sample_spec() const {
    return config_.output_sample_spec;
}

core::nanoseconds_t TranscoderSink::frame_length() const {
    return 0;
}

bool TranscoderSink::has_state() const {
    return false;
}

bool TranscoderSink::has_latency() const {
    return false;
}

bool TranscoderSink::has_clock() const {
    return false;
}

status::StatusCode TranscoderSink::write(audio::Frame& frame) {
    roc_panic_if(init_status_ != status::StatusOK);

    return frame_writer_->write(frame);
}

status::StatusCode TranscoderSink::flush() {
    return status::StatusOK;
}

status::StatusCode TranscoderSink::close() {
    return status::StatusOK;
}

void TranscoderSink::dispose() {
    arena().dispose_object(*this);
}

} // namespace pipeline
} // namespace roc
