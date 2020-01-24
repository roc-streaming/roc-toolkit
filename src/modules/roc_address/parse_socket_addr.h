/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/parse_socket_addr.h
//! @brief Parse address from string.

#ifndef ROC_ADDRESS_PARSE_SOCKET_ADDR_H_
#define ROC_ADDRESS_PARSE_SOCKET_ADDR_H_

#include "roc_address/socket_addr.h"

namespace roc {
namespace address {

//! Parse host and port.
//!
//! @remarks
//!  The @p host string should be in one of the following forms:
//!   - "IPv4"    e.g. "1.2.3.4"
//!   - "[IPv6]"  e.g. "[::1]"
//!
//! @returns
//!  false if @p host can't be parsed.
bool parse_socket_addr_host_port(const char* host, int port, SocketAddr& addr);

//! Parse multicast interface address on which to join to the multicast group.
//!
//! @remarks
//!  The @p input string should be in one of the following forms:
//!   - "IPv4"    e.g. "1.2.3.4"
//!   - "[IPv6]"  e.g. "[::1]"
//!
//! @returns
//!  false if @p input can't be parsed;
//!  false if @p addr is not multicast;
//!  false if @p miface represents IP address with a version other than
//!  IP version of @p addr.
bool parse_socket_addr_miface(const char* miface, SocketAddr& addr);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_PARSE_SOCKET_ADDR_H_
