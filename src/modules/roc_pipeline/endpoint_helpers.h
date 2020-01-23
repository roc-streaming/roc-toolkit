/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/endpoint_helpers.h
//! @brief Validate endpoint protocols consistency.

#ifndef ROC_PIPELINE_ENDPOINT_HELPERS_H_
#define ROC_PIPELINE_ENDPOINT_HELPERS_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_type.h"
#include "roc_packet/fec.h"

namespace roc {
namespace pipeline {

//! Validate endpoint type and protocol.
bool validate_endpoint(address::EndpointType type, address::EndpointProtocol proto);

//! Validate consistency of the endpoint and the pipeline FEC scheme.
bool validate_endpoint_and_pipeline_consistency(packet::FecScheme pipeline_fec_scheme,
                                                address::EndpointType type,
                                                address::EndpointProtocol proto);

//! Validate consistency of the two endpoints.
bool validate_endpoint_pair_consistency(address::EndpointProtocol source_proto,
                                        address::EndpointProtocol repair_proto);

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_ENDPOINT_HELPERS_H_
