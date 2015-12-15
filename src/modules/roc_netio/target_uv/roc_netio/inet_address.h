/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_netio/target_uv/roc_netio/inet_address.h
//! @brief Socket address helpers.

#ifndef ROC_NETIO_INET_ADDRESS_H_
#define ROC_NETIO_INET_ADDRESS_H_

#include <uv.h>

#include "roc_datagram/address.h"

namespace roc {
namespace netio {

//! Convert datagram::Address to sockaddr_in.
void to_inet_address(const datagram::Address&, sockaddr_in& result);

//! Convert sockaddr_in to datagram::Address.
void from_inet_address(const sockaddr_in&, datagram::Address& result);

//! Parse address from string.
//! @remarks
//!  @p string should be in form "[<IP>]:<PORT>".
//! @returns
//!  false if string can't be parsed or hostname can't be resolved.
bool parse_address(const char* string, datagram::Address& result);

} // namespace netio
} // namespace roc

#endif // ROC_NETIO_INET_ADDRESS_H_
