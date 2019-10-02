/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/proto.h
//! @brief Protocol functions.

#ifndef ROC_ADDRESS_PROTO_H_
#define ROC_ADDRESS_PROTO_H_

#include "roc_address/endpoint_enums.h"
#include "roc_packet/fec.h"

namespace roc {
namespace address {

//! Get FEC scheme for given protocol.
packet::FECScheme proto_fec_scheme(EndpointProtocol proto);

//! Get default port number for given protocol.
int proto_default_port(EndpointProtocol proto);

//! Check whether given protocol supports path in URI.
bool proto_supports_path(EndpointProtocol proto);

//! Get string name of the protocol.
const char* proto_to_str(EndpointProtocol proto);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_PROTO_H_
