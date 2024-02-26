/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sndfile/roc_sndio/sndfile_backend.h
//! @brief SndFile backend.

#ifndef ROC_SNDIO_SNDFILE_BACKEND_H_
#define ROC_SNDIO_SNDFILE_BACKEND_H_

#include <sndfile.h>

#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_sndio/ibackend.h"

namespace roc {
namespace sndio {

//! Sndfile backend.
class SndfileBackend : public IBackend, core::NonCopyable<> {
public:
    SndfileBackend();

    //! Append supported drivers to the list.
    virtual void discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list);

    //! Create and open a sink or source.
    virtual IDevice* open_device(DeviceType device_type,
                                 DriverType driver_type,
                                 const char* driver,
                                 const char* path,
                                 const Config& config,
                                 core::IArena& arena);
    //! Returns name of backend.
    virtual const char* name() const;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SNDFILE_BACKEND_H_
