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
#include "roc_core/attributes.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_sndio/device_state.h"
#include "roc_sndio/device_type.h"
#include "roc_status/status_code.h"

namespace roc {
namespace sndio {

class ISink;
class ISource;

//! Base interface for sinks and sources.
//!
//! Under the hood device can be:
//!  - "hardware device" (e.g. PulseAudio source or sink)
//!  - "file device" (e.g. WAV file)
//!  - "network device" (e.g. sender or receiver pipeline)
//!
//! Hardware and file devices are implemented in roc_sndio, and
//! network devices are implemented in roc_pipeline.
//!
//! sndio::Pump is a class that copies stream from ISource to ISink
//! regardless of the device kind, e.g. from file to network sender,
//! or from network receiver to speakers.
class IDevice {
public:
    virtual ~IDevice();

    //! Get device type (sink or source).
    virtual DeviceType type() const = 0;

    //! Cast IDevice to ISink.
    //! @remarks
    //!  If device is not sink, returns NULL.
    virtual ISink* to_sink() = 0;

    //! Cast IDevice to ISink.
    //! @remarks
    //!  If device is not source, returns NULL.
    virtual ISource* to_source() = 0;

    //! Get sample specification of the device.
    //! Frame written to or read from the device should use this specification.
    virtual audio::SampleSpec sample_spec() const = 0;

    //! Check if the device supports state updates.
    //! @remarks
    //!  If true, state() returns current state, and pause() and resume()
    //!  can be used to change state.
    virtual bool has_state() const = 0;

    //! Get device state.
    //! @remarks
    //!  Device may change state by itself (e.g if underlying hardware device changes
    //!  state), and also after pause() or resume() is invoked.
    //! @note
    //!  Makes sense only if has_state() is true.
    virtual DeviceState state() const;

    //! Pause device.
    //! @remarks
    //!  This operation makes sense for hardware and network devices.
    //!  For non-hardware devices it's usually no-op.
    //!  After device is paused, there should be no I/O until it's resumed.
    //! @note
    //!  Makes sense only if has_state() is true.
    virtual ROC_ATTR_NODISCARD status::StatusCode pause();

    //! Resume device after pause.
    //! @remarks
    //!  This operation makes sense for hardware and network devices.
    //!  For non-hardware devices it's usually no-op.
    //!  After device is paused, it should be resumed to do I/O again.
    //! @note
    //!  Makes sense only if has_state() is true.
    virtual ROC_ATTR_NODISCARD status::StatusCode resume();

    //! Check if the device supports latency reports.
    //! @remarks
    //!  If true, latency() returns meaningful values.
    virtual bool has_latency() const = 0;

    //! Get latency of the device.
    //! @remarks
    //!  For sink, represents time between sample is written to sink and time it
    //!  reaches its destination (e.g. played on speakers).
    //!  For source, represents time between sample is captured on its origin
    //!  (e.g. microphone) and time when it can be read from device.
    //! @note
    //!  This value is highly platform- and driver-dependent, e.g. some drivers
    //!  may take ADC/DAC into account, some not, etc.
    //! @note
    //!  Makes sense only if has_latency() is true.
    virtual core::nanoseconds_t latency() const;

    //! Check if the device has own clock.
    //! @remarks
    //!  If true, writing to or reading from device is blocking operation,
    //!  driven by internal device clock.
    //!  If false, the user is responsible to maintain the clock and
    //!  perform writes or read in-time.
    virtual bool has_clock() const = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_IDEVICE_H_
