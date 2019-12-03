/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/converter.h
//! @brief Converter pipeline.

#ifndef ROC_PIPELINE_CONVERTER_H_
#define ROC_PIPELINE_CONVERTER_H_

#include "roc_audio/iresampler.h"
#include "roc_audio/null_writer.h"
#include "roc_audio/poison_writer.h"
#include "roc_audio/profiling_writer.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_profile.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/unique_ptr.h"
#include "roc_pipeline/config.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {

//! Converter pipeline.
class Converter : public sndio::ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    Converter(const ConverterConfig& config,
              audio::IWriter* output_writer,
              core::BufferPool<audio::sample_t>& pool,
              core::IAllocator& allocator);

    //! Check if the pipeline was successfully constructed.
    bool valid();

    //! Get sink sample rate.
    virtual size_t sample_rate() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    audio::NullWriter null_writer_;

    core::UniquePtr<audio::PoisonWriter> resampler_poisoner_;
    core::UniquePtr<audio::ResamplerWriter> resampler_writer_;
    core::UniquePtr<audio::IResampler> resampler_;

    core::UniquePtr<audio::ProfilingWriter> profiler_;

    core::UniquePtr<audio::PoisonWriter> pipeline_poisoner_;

    audio::IWriter* audio_writer_;

    ConverterConfig config_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_CONVERTER_H_
