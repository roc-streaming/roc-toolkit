/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/backend_dispatcher.h
//! @brief Backend dispatcher.

#ifndef ROC_SNDIO_BACKEND_DISPATCHER_H_
#define ROC_SNDIO_BACKEND_DISPATCHER_H_

#include "roc_address/io_uri.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"
#include "roc_core/string_list.h"
#include "roc_sndio/device_type.h"
#include "roc_sndio/driver.h"
#include "roc_sndio/ibackend.h"
#include "roc_sndio/isink.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! Backend dispatcher.
class BackendDispatcher : public core::NonCopyable<> {
public:
    //! Initialize.
    BackendDispatcher(core::IArena& arena);

    //! Create and open default sink.
    ISink* open_default_sink(const Config& config);

    //! Create and open default source.
    ISource* open_default_source(const Config& config);

    //! Create and open a sink.
    ISink*
    open_sink(const address::IoUri& uri, const char* force_format, const Config& config);

    //! Create and open a source.
    ISource* open_source(const address::IoUri& uri,
                         const char* force_format,
                         const Config& config);

    //! Get all supported URI schemes.
    ROC_ATTR_NODISCARD bool get_supported_schemes(core::StringList& result);

    //! Get all supported file formats.
    ROC_ATTR_NODISCARD bool get_supported_formats(core::StringList& result);

private:
    IDevice* open_default_device_(DeviceType device_type, const Config& config);

    IDevice* open_device_(DeviceType device_type,
                          DriverType driver_type,
                          const char* driver_name,
                          const char* path,
                          const Config& config);

    core::IArena& arena_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_BACKEND_DISPATCHER_H_
