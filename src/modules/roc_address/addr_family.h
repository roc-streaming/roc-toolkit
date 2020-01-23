/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/addr_family.h
//! @brief Address family.

#ifndef ROC_ADDRESS_ADDR_FAMILY_H_
#define ROC_ADDRESS_ADDR_FAMILY_H_

namespace roc {
namespace address {

//! Address family.
enum AddrFamily {
    Family_Unknown = 0, //!< Invalid.
    Family_IPv4,        //!< IPv4.
    Family_IPv6         //!< IPv6.
};

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_ADDR_FAMILY_H_
