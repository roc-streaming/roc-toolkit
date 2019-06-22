/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_uv/roc_netio/handle.h
//! @brief Handle.

#ifndef ROC_NETIO_HANDLE_H_
#define ROC_NETIO_HANDLE_H_

#include "roc_packet/address.h"

namespace roc {
namespace netio {

//! Network handle.
struct Handle {
    //! Construct empty handle.
    Handle()
        : data(NULL)
        , fn(NULL) {
    }

    //! Data to call callback with.
    void* data;

    //! Handle callback.
    void (*fn)(void*, packet::Address&);
};

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_HANDLE_H_
