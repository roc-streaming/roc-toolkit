/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iclose_handler.h
//! @brief Close handler.

#ifndef ROC_NETIO_ICLOSE_HANDLER_H_
#define ROC_NETIO_ICLOSE_HANDLER_H_

#include "roc_netio/basic_port.h"

namespace roc {
namespace netio {

//! Close handler interface.
class ICloseHandler {
public:
    virtual ~ICloseHandler();

    //! Handle asynchronously closed port.
    //!
    //! @remarks
    //!  - After this call closed port should not be used.
    //!  - Should be called from the event loop thread.
    virtual void handle_closed(BasicPort&) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ICLOSE_HANDLER_H_
