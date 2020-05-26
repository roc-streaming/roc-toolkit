/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/converter_sink.h
//! @brief Converter sink pipeline.

#ifndef ROC_PIPELINE_CONVERTER_SINK_H_
#define ROC_PIPELINE_CONVERTER_SINK_H_

#include "roc_audio/iresampler.h"
#include "roc_audio/null_writer.h"
#include "roc_audio/poison_writer.h"
#include "roc_audio/profiling_writer.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/scoped_ptr.h"
#include "roc_pipeline/config.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Converter sink pipeline.
//! @remarks
//!  - input: frames
//!  - output: frames
class ConverterSink : public sndio::ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    ConverterSink(const ConverterConfig& config,
                  audio::IWriter* output_writer,
                  core::BufferPool<audio::sample_t>& pool,
                  core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid();

    //! Get sink sample rate.
    virtual size_t sample_rate() const;

    //! Get number of channels for the sink.
    virtual size_t num_channels() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    audio::NullWriter null_writer_;

    core::ScopedPtr<audio::PoisonWriter> resampler_poisoner_;
    core::ScopedPtr<audio::ResamplerWriter> resampler_writer_;
    core::ScopedPtr<audio::IResampler> resampler_;

    core::ScopedPtr<audio::PoisonWriter> pipeline_poisoner_;

    core::ScopedPtr<audio::ProfilingWriter> profiler_;

    audio::IWriter* audio_writer_;

    const ConverterConfig config_;
    const size_t num_channels_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONVERTER_SINK_H_
