/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_pulseaudio/roc_sndio/pulseaudio_source.h
//! @brief PulseAudio source.

#ifndef ROC_SNDIO_PULSEAUDIO_SOURCE_H_
#define ROC_SNDIO_PULSEAUDIO_SOURCE_H_

#include "roc_sndio/isource.h"
#include "roc_sndio/pulseaudio_device.h"

namespace roc {
namespace sndio {

//! PulseAudio source,
class PulseaudioSource : public ISource, public PulseaudioDevice {
public:
    //! Initialize.
    PulseaudioSource(const Config& config);

    ~PulseaudioSource();

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

    //! Get sample specification of the source.
    virtual audio::SampleSpec sample_spec() const;

    //! Get latency of the source.
    virtual core::nanoseconds_t latency() const;

    //! Check if the source supports latency reports.
    virtual bool has_latency() const;

    //! Check if the source has own clock.
    virtual bool has_clock() const;

    //! Adjust source clock to match consumer clock.
    virtual void reclock(core::nanoseconds_t timestamp);

    //! Read frame.
    virtual bool read(audio::Frame& frame);
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_PULSEAUDIO_SOURCE_H_
