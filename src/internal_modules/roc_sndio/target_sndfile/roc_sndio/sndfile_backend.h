/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sndfile/roc_sndio/sndfile_backend.h
//! @brief Sndfile backend.

#ifndef ROC_SNDIO_SNDFILE_BACKEND_H_
#define ROC_SNDIO_SNDFILE_BACKEND_H_

#include "roc_core/noncopyable.h"
#include "roc_sndio/ibackend.h"

namespace roc {
namespace sndio {

//! Sndfile backend.
class SndfileBackend : public IBackend, core::NonCopyable<> {
public:
    SndfileBackend();

    //! Returns name of backend.
    virtual const char* name() const;

    //! Append supported drivers to the list.
    virtual void discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list);

    //! Create and open a sink or source.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    open_device(DeviceType device_type,
                DriverType driver_type,
                const char* driver,
                const char* path,
                const IoConfig& io_config,
                audio::FrameFactory& frame_factory,
                core::IArena& arena,
                IDevice** result);
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SNDFILE_BACKEND_H_
