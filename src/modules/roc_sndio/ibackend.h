/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/ibackend.h
//! @brief Backend interface.

#ifndef ROC_SNDIO_IBACKEND_H_
#define ROC_SNDIO_IBACKEND_H_

#include "roc_core/array.h"
#include "roc_core/iallocator.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/string_list.h"
#include "roc_sndio/config.h"
#include "roc_sndio/driver.h"
#include "roc_sndio/isink.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {

//! Backend interface.

//! Allows three cases of driver and device combinations.
//! [1. Driver is NULL and device is NULL, iterate through default drivers and perform
//! open_sink()/open_source() with appropriate backend until successful.
//! 2. Driver is NULL and device is not NULL, open_sink()/open_source is called
//! with appropriate backend.
//! 3. When driver is not NULL and device is not NULL, open_sink()/open_source()
//! is performed with appropriate backend for given driver and device.]
class IBackend {
public:
    virtual ~IBackend();

    //! Flags to filter by device type.
    enum FilterFlags {

        //! Input or output may be a file.
        FilterFile = (1 << 0),
        //! Input or output may be a device.
        FilterDevice = (1 << 1)
    };

    //! Create and open a sink.
    virtual ISink* open_sink(core::IAllocator& allocator,
                             const char* driver,
                             const char* output,
                             const Config& config,
                             int filter_flags) = 0;

    //! Create and open a source.
    virtual ISource* open_source(core::IAllocator& allocator,
                                 const char* driver,
                                 const char* input,
                                 const Config& config,
                                 int filter_flags) = 0;

    //! Append supported drivers to the list.
    virtual bool get_drivers(core::Array<DriverInfo>&, int filter_flags) = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_IBACKEND_H_
