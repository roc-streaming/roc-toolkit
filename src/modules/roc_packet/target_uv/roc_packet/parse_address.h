/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/target_uv/roc_packet/parse_address.h
//! @brief Socket address helpers.

#ifndef ROC_PACKET_PARSE_ADDRESS_H_
#define ROC_PACKET_PARSE_ADDRESS_H_

#include "roc_packet/address.h"

namespace roc {
namespace packet {

//! Parse address from string.
//!
//! @remarks
//!  The input string should be in one of the following forms:
//!   - ":PORT", e.g. ":123"
//!   - "IPv4:PORT", e.g. "1.2.3.4:123"
//!   - "[IPv6]:PORT", e.g. "[::1]:123"
//!
//! @returns
//!  false if string can't be parsed.
bool parse_address(const char* string, packet::Address& result);

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_PARSE_ADDRESS_H_
