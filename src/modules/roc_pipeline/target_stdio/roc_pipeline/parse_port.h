/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/target_stdio/roc_pipeline/parse_port.h
//! @brief Parse port from string.

#ifndef ROC_PIPELINE_PARSE_PORT_H_
#define ROC_PIPELINE_PARSE_PORT_H_

#include "roc_address/interface.h"
#include "roc_pipeline/config.h"

namespace roc {
namespace pipeline {

//! Parse port from string.
//!
//! @remarks
//!  The input string should be in one of the following forms:
//!   - "PROTO::PORT"        e.g. "rtp::123"
//!   - "PROTO:IPv4:PORT"    e.g. "rtp:1.2.3.4:123"
//!   - "PROTO:[IPv6]:PORT"  e.g. "rtp:[::1]:123"
//!
//! @returns
//!  false if string can't be parsed.
bool parse_port(address::Interface iface, const char* string, PortConfig& result);

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_PARSE_PORT_H_
