/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/endpoint_type_to_str.h
//! @brief Convert endpoint type to string.

#ifndef ROC_ADDRESS_ENDPOINT_TYPE_TO_STR_H_
#define ROC_ADDRESS_ENDPOINT_TYPE_TO_STR_H_

#include "roc_address/endpoint_enums.h"

namespace roc {
namespace address {

//! Convert endpoint type to string.
const char* endpoint_type_to_str(EndpointType);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_ENDPOINT_TYPE_TO_STR_H_
