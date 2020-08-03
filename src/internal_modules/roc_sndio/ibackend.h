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

#include "roc_core/array.h"
#include "roc_core/iallocator.h"
#include "roc_sndio/config.h"
#include "roc_sndio/driver.h"
#include "roc_sndio/isink.h"
#include "roc_sndio/isource.h"
#include "roc_sndio/terminal.h"

namespace roc {
namespace sndio {

//! Backend interface.
class IBackend {
public:
    virtual ~IBackend();

    //! Create and open a sink or source.
    virtual ITerminal* open_terminal(core::IAllocator& allocator,
                                     TerminalType terminal_type,
                                     DriverType driver_type,
                                     const char* driver,
                                     const char* path,
                                     const Config& config) = 0;

    //! Append supported drivers to the list.
    virtual bool get_drivers(core::Array<DriverInfo>& driver_list) = 0;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_IBACKEND_H_
