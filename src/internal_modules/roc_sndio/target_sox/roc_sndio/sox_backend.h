/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/target_sox/roc_sndio/sox_backend.h
//! @brief SoX backend.

#ifndef ROC_SNDIO_SOX_BACKEND_H_
#define ROC_SNDIO_SOX_BACKEND_H_

#include <sox.h>

#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_sndio/ibackend.h"

namespace roc {
namespace sndio {

//! SoX backend.
class SoxBackend : public IBackend, core::NonCopyable<> {
public:
    SoxBackend();

    //! Set internal SoX frame size.
    //! @remarks
    //!  Number of samples for all channels.
    void set_frame_size(core::nanoseconds_t frame_length,
                        const audio::SampleSpec& sample_spec);

    //! Append supported drivers to the list.
    virtual void discover_drivers(core::Array<DriverInfo, MaxDrivers>& driver_list);

    //! Create and open a sink or source.
    virtual IDevice* open_device(DeviceType device_type,
                                 DriverType driver_type,
                                 const char* driver,
                                 const char* path,
                                 const Config& config,
                                 core::IArena& arena);

private:
    bool first_created_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_BACKEND_H_
