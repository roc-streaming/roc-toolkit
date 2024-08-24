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

#include "roc_core/noncopyable.h"
#include "roc_sndio/ibackend.h"

namespace roc {
namespace sndio {

//! SoX backend.
class SoxBackend : public IBackend, core::NonCopyable<> {
public:
    SoxBackend();

    //! Returns name of backend.
    virtual const char* name() const;

    //! Append supported drivers to the list.
    virtual ROC_ATTR_NODISCARD bool
    discover_drivers(core::Array<DriverInfo, MaxDrivers>& result);

    //! Append supported formats to the list.
    virtual ROC_ATTR_NODISCARD bool
    discover_formats(core::Array<FormatInfo, MaxFormats>& result);

    //! Append supported groups of sub-formats to the list.
    virtual ROC_ATTR_NODISCARD bool discover_subformat_groups(core::StringList& result);

    //! Append supported sub-formats of a group to the list.
    virtual ROC_ATTR_NODISCARD bool discover_subformats(const char* group,
                                                        core::StringList& result);

    //! Create and open a sink or source.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    open_device(DeviceType device_type,
                const char* driver,
                const char* path,
                const IoConfig& io_config,
                audio::FrameFactory& frame_factory,
                core::IArena& arena,
                IDevice** result);
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_SOX_BACKEND_H_
