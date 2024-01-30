/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/wav_sink.h
//! @brief WAV sink.

#ifndef ROC_SNDIO_WAV_SINK_H_
#define ROC_SNDIO_WAV_SINK_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"
#include "roc_sndio/config.h"
#include "roc_sndio/isink.h"

#include "wav_header.h"

namespace roc {
namespace sndio {

//! WAV sink.
//! @remarks
//!  Writes samples to output file.
class WavSink : public ISink, public core::NonCopyable<> {
public:
    //! Initialize.
    WavSink(core::IArena& arena, const Config& config);

    virtual ~WavSink();

    //! Check if the object was successfully constructed.
    bool is_valid() const;

    //! Open output file.
    //!
    //! @b Parameters
    //!  - @p path is output file or device name, "-" for stdout.
    //!
    //! @remarks
    //!  If @p path is NULL, defaults are used.
    bool open(const char* path);

    //! Cast IDevice to ISink.
    virtual ISink* to_sink();

    //! Cast IDevice to ISink.
    virtual ISource* to_source();

    //! Get device type.
    virtual DeviceType type() const;

    //! Get device state.
    virtual DeviceState state() const;

    //! Pause reading.
    virtual void pause();

    //! Resume paused reading.
    virtual bool resume();

    //! Restart reading from the beginning.
    virtual bool restart();

    //! Get sample specification of the sink.
    virtual audio::SampleSpec sample_spec() const;

    //! Get latency of the sink.
    virtual core::nanoseconds_t latency() const;

    //! Check if the sink supports latency reports.
    virtual bool has_latency() const;

    //! Check if the sink has own clock.
    virtual bool has_clock() const;

    //! Write audio frame.
    virtual void write(audio::Frame& frame);

private:
    bool open_(const char* path);
    void close_();

    FILE* output_file_;
    WavHeader header_;

    bool valid_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_WAV_SINK_H_
