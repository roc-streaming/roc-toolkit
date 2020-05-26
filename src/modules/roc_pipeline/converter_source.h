/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/converter_source.h
//! @brief Converter source pipeline.

#ifndef ROC_PIPELINE_CONVERTER_SOURCE_H_
#define ROC_PIPELINE_CONVERTER_SOURCE_H_

#include "roc_audio/iresampler.h"
#include "roc_audio/poison_reader.h"
#include "roc_audio/profiling_reader.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/resampler_reader.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/scoped_ptr.h"
#include "roc_pipeline/config.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {

//! Converter source pipeline.
//! @remarks
//!  - input: frames
//!  - output: frames
class ConverterSource : public sndio::ISource, public core::NonCopyable<> {
public:
    //! Initialize.
    ConverterSource(const ConverterConfig& config,
                    sndio::ISource& input_source,
                    core::BufferPool<audio::sample_t>& pool,
                    core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid();

    //! Get sink sample rate.
    virtual size_t sample_rate() const;

    //! Get number of channels for the source.
    virtual size_t num_channels() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Get current source state.
    virtual State state() const;

    //! Pause reading.
    virtual void pause();

    //! Resume paused reading.
    virtual bool resume();

    //! Restart reading from the beginning.
    virtual bool restart();

    //! Read frame.
    virtual bool read(audio::Frame&);

private:
    core::ScopedPtr<audio::PoisonReader> resampler_poisoner_;
    core::ScopedPtr<audio::ResamplerReader> resampler_reader_;
    core::ScopedPtr<audio::IResampler> resampler_;

    core::ScopedPtr<audio::PoisonReader> pipeline_poisoner_;
    core::ScopedPtr<audio::ProfilingReader> profiler_;

    sndio::ISource& input_source_;
    audio::IReader* audio_reader_;

    ConverterConfig config_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONVERTER_SOURCE_H_
