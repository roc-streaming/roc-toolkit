/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/endpoint_type.h
//! @brief Network endpoint type.

#ifndef ROC_ADDRESS_ENDPOINT_TYPE_H_
#define ROC_ADDRESS_ENDPOINT_TYPE_H_

namespace roc {
namespace address {

//! Network endpoint type.
enum EndpointType {
    //! Session initiation and control.
    EndType_Control,

    //! Audio source packets.
    EndType_AudioSource,

    //! Audio repair packets.
    EndType_AudioRepair,

    //! Number of endpoint types.
    EndType_Max
};

//! Convert endpoint type to string.
const char* endpoint_type_to_str(EndpointType);

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_ENDPOINT_TYPE_H_
