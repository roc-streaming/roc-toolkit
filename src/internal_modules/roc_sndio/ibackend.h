/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/ibackend.h
//! @brief Backend interface.

#ifndef ROC_SNDIO_IBACKEND_H_
#define ROC_SNDIO_IBACKEND_H_

#include "roc_audio/frame_factory.h"
#include "roc_core/array.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_sndio/config.h"
#include "roc_sndio/device_type.h"
#include "roc_sndio/driver.h"
#include "roc_sndio/idevice.h"
#include "roc_status/status_code.h"

namespace roc {
namespace sndio {

//! Maximum number of backends.
static const size_t MaxBackends = 8;

//! Backend interface.
class IBackend {
public:
    virtual ~IBackend();

    //! Returns name of backend.
    virtual const char* name() const = 0;

    //! Append supported drivers to the list.
    virtual void discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list) = 0;

    //! Create and open a sink or source.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    open_device(DeviceType device_type,
                DriverType driver_type,
                const char* driver,
                const char* path,
                const Config& config,
                audio::FrameFactory& frame_factory,
                core::IArena& arena,
                IDevice** result) = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_IBACKEND_H_
