/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_libuv/roc_netio/iclose_handler.h
//! @brief Close handler interface.

#ifndef ROC_NETIO_ICLOSE_HANDLER_H_
#define ROC_NETIO_ICLOSE_HANDLER_H_

namespace roc {
namespace netio {

class BasicPort;

//! Close handler interface.
class ICloseHandler {
public:
    virtual ~ICloseHandler();

    //! Handle completion of asynchronous closing of given port.
    //! @remarks
    //!  After this call, the closed port should not be used.
    //! @note
    //!  This method os called from the network loop thread.
    virtual void handle_close_completed(BasicPort& port, void* arg) = 0;
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_ICLOSE_HANDLER_H_
