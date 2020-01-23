/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_peer/validate.h
//! @brief Validate endpoint protocols consistency.

#ifndef ROC_PEER_VALIDATE_H_
#define ROC_PEER_VALIDATE_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_type.h"
#include "roc_packet/fec.h"

namespace roc {
namespace peer {

//! Validate consistency of the endpoint protocol and the FEC scheme.
bool validate_transport_endpoint(packet::FecScheme fec_scheme,
                                 address::EndpointType type,
                                 address::EndpointProtocol proto);

//! Validate consistency of the two endpoints protocols and FEC scheme.
bool validate_transport_endpoint_pair(packet::FecScheme fec_scheme,
                                      address::EndpointProtocol source_proto,
                                      address::EndpointProtocol repair_proto);

} // namespace peer
} // namespace roc

#endif // ROC_PEER_VALIDATE_H_