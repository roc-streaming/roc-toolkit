/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/transcoder_source.h
//! @brief Transcoder source pipeline.

#ifndef ROC_PIPELINE_TRANSCODER_SOURCE_H_
#define ROC_PIPELINE_TRANSCODER_SOURCE_H_

#include "roc_audio/channel_mapper_reader.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/iresampler.h"
#include "roc_audio/processor_map.h"
#include "roc_audio/profiling_reader.h"
#include "roc_audio/resampler_reader.h"
#include "roc_core/ipool.h"
#include "roc_core/optional.h"
#include "roc_core/scoped_ptr.h"
#include "roc_pipeline/config.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Transcoder source pipeline.
//! @remarks
//!  - input: frames
//!  - output: frames
class TranscoderSource : public sndio::ISource, public core::NonCopyable<> {
public:
    //! Initialize.
    TranscoderSource(const TranscoderConfig& config,
                     sndio::ISource& input_source,
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

    //! Get sample specification of the source.
    virtual audio::SampleSpec sample_spec() const;

    //! Get recommended frame length of the source.
    virtual core::nanoseconds_t frame_length() const;

    //! Check if the source supports state updates.
    virtual bool has_state() const;

    //! Get current source state.
    virtual sndio::DeviceState state() const;

    //! Pause source.
    virtual ROC_ATTR_NODISCARD status::StatusCode pause();

    //! Resume source.
    virtual ROC_ATTR_NODISCARD status::StatusCode resume();

    //! Check if the source supports latency reports.
    virtual bool has_latency() const;

    //! Get latency of the source.
    virtual core::nanoseconds_t latency() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Restart reading from beginning.
    virtual ROC_ATTR_NODISCARD status::StatusCode rewind();

    //! Adjust sessions clock to match consumer clock.
    virtual void reclock(core::nanoseconds_t playback_time);

    //! Read frame.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(audio::Frame& frame,
         packet::stream_timestamp_t duration,
         audio::FrameReadMode mode);

    //! Explicitly close the source.
    virtual ROC_ATTR_NODISCARD status::StatusCode close();

    //! Destroy object and return memory to arena.
    virtual void dispose();

private:
    audio::FrameFactory frame_factory_;

    core::Optional<audio::ChannelMapperReader> channel_mapper_reader_;

    core::Optional<audio::ResamplerReader> resampler_reader_;
    core::SharedPtr<audio::IResampler> resampler_;

    core::Optional<audio::ProfilingReader> profiler_;

    sndio::ISource& input_source_;
    audio::IFrameReader* frame_reader_;

    TranscoderConfig config_;

    status::StatusCode init_status_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TRANSCODER_SOURCE_H_
