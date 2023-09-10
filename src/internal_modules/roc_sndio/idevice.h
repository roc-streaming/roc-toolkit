/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/idevice.h
//! @brief Device interface.

#ifndef ROC_SNDIO_IDEVICE_H_
#define ROC_SNDIO_IDEVICE_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_sndio/device_state.h"
#include "roc_sndio/device_type.h"

namespace roc {
namespace sndio {

//! Base interface for sinks and sources.
class IDevice {
public:
    virtual ~IDevice();

    //! Get device type.
    virtual DeviceType type() const = 0;

    //! Get device state.
    virtual DeviceState state() const = 0;

    //! Pause device.
    virtual void pause() = 0;

    //! Resume device after pause.
    //! @returns
    //!  false if an error occured.
    virtual bool resume() = 0;

    //! Restart device.
    //! @remarks
    //!  If device was paused, it's automatically resumed.
    //! @returns
    //!  false if an error occured.
    virtual bool restart() = 0;

    //! Get sample specification of the device.
    virtual audio::SampleSpec sample_spec() const = 0;

    //! Get latency of the device.
    virtual core::nanoseconds_t latency() const = 0;

    //! Check if the device supports latency reports.
    virtual bool has_latency() const = 0;

    //! Check if the device has own clock.
    virtual bool has_clock() const = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_IDEVICE_H_
