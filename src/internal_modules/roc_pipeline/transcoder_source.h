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
                     core::IPool& buffer_pool,
                     core::IArena& arena);

    //! Check if the pipeline was successfully constructed.
    status::StatusCode init_status() const;

    //! Cast IDevice to ISink.
    virtual sndio::ISink* to_sink();

    //! Cast IDevice to ISink.
    virtual sndio::ISource* to_source();

    //! Get device type.
    virtual sndio::DeviceType type() const;

    //! Get device state.
    virtual sndio::DeviceState state() const;

    //! Pause reading.
    virtual void pause();

    //! Resume paused reading.
    virtual bool resume();

    //! Restart reading from the beginning.
    virtual bool restart();

    //! Get sample specification of the source.
    virtual audio::SampleSpec sample_spec() const;

    //! Get latency of the source.
    virtual core::nanoseconds_t latency() const;

    //! Check if the source supports latency reports.
    virtual bool has_latency() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Adjust source clock to match consumer clock.
    virtual void reclock(core::nanoseconds_t timestamp);

    //! Read frame.
    virtual bool read(audio::Frame&);

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
