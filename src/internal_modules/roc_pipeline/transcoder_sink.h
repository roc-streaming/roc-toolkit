/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/transcoder_sink.h
//! @brief Transcoder sink pipeline.

#ifndef ROC_PIPELINE_TRANSCODER_SINK_H_
#define ROC_PIPELINE_TRANSCODER_SINK_H_

#include "roc_audio/channel_mapper_writer.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/null_writer.h"
#include "roc_audio/processor_map.h"
#include "roc_audio/profiling_writer.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/ipool.h"
#include "roc_core/optional.h"
#include "roc_core/scoped_ptr.h"
#include "roc_pipeline/config.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Transcoder sink pipeline.
//! @remarks
//!  - input: frames
//!  - output: frames
class TranscoderSink : public sndio::ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    TranscoderSink(const TranscoderConfig& config,
                   audio::IFrameWriter* output_writer,
                   audio::ProcessorMap& processor_map,
                   core::IPool& frame_pool,
                   core::IPool& frame_buffer_pool,
                   core::IArena& arena);

    //! Check if the pipeline was successfully constructed.
    status::StatusCode init_status() const;

    //! Get type (sink or source).
    virtual sndio::DeviceType type() const;

    //! Try to cast to ISink.
    virtual sndio::ISink* to_sink();

    //! Try to cast to ISource.
    virtual sndio::ISource* to_source();

    //! Get sample specification of the sink.
    virtual audio::SampleSpec sample_spec() const;

    //! Get recommended frame length of the sink.
    virtual core::nanoseconds_t frame_length() const;

    //! Check if the sink supports state updates.
    virtual bool has_state() const;

    //! Check if the sink supports latency reports.
    virtual bool has_latency() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(audio::Frame& frame);

    //! Flush buffered data, if any.
    virtual ROC_ATTR_NODISCARD status::StatusCode flush();

    //! Explicitly close the sink.
    virtual ROC_ATTR_NODISCARD status::StatusCode close();

    //! Destroy object and return memory to arena.
    virtual void dispose();

private:
    audio::FrameFactory frame_factory_;

    audio::NullWriter null_writer_;

    core::Optional<audio::ChannelMapperWriter> channel_mapper_writer_;

    core::Optional<audio::ResamplerWriter> resampler_writer_;
    core::SharedPtr<audio::IResampler> resampler_;

    core::Optional<audio::ProfilingWriter> profiler_;

    audio::IFrameWriter* frame_writer_;

    TranscoderConfig config_;

    status::StatusCode init_status_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TRANSCODER_SINK_H_
