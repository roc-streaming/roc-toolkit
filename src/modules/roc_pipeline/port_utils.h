/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/port_utils.h
//! @brief Validate port protocols consistency.

#ifndef ROC_PIPELINE_PORT_UTILS_H_
#define ROC_PIPELINE_PORT_UTILS_H_

#include "roc_packet/fec.h"
#include "roc_pipeline/port.h"

namespace roc {
namespace pipeline {

//! Get FEC scheme for given protocol.
packet::FecScheme port_fec_scheme(PortProtocol proto);

//! Validate consistency of a single port and FEC scheme.
bool validate_port(packet::FecScheme fec_scheme,
                   PortProtocol port_protocol,
                   PortType port_type);

//! Validate consistency of two ports and FEC scheme.
bool validate_ports(packet::FecScheme fec_scheme,
                    PortProtocol source_port,
                    PortProtocol repair_port);

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_PORT_UTILS_H_
