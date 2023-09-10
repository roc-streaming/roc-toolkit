/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_pulseaudio/roc_sndio/pulseaudio_sink.h
//! @brief PulseAudio sink.

#ifndef ROC_SNDIO_PULSEAUDIO_SINK_H_
#define ROC_SNDIO_PULSEAUDIO_SINK_H_

#include "roc_sndio/isink.h"
#include "roc_sndio/pulseaudio_device.h"

namespace roc {
namespace sndio {

//! PulseAudio sink,
class PulseaudioSink : public ISink, public PulseaudioDevice {
public:
    //! Initialize.
    PulseaudioSink(const Config& config);

    ~PulseaudioSink();

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
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PULSEAUDIO_SINK_H_
