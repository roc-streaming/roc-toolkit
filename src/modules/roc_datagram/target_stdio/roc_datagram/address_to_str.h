/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_datagram/target_stdio/roc_datagram/address_to_str.h
//! @brief Convert network address to string.

#ifndef ROC_DATAGRAM_ADDRESS_TO_STR_H_
#define ROC_DATAGRAM_ADDRESS_TO_STR_H_

#include "roc_core/string_buffer.h"
#include "roc_datagram/address.h"

namespace roc {
namespace datagram {

//! Convert network address to string.
class address_to_str : public core::StringBuffer<32> {
public:
    //! Construct.
    explicit address_to_str(const Address&);
};

} // namespace datagram
} // namespace roc

#endif // ROC_DATAGRAM_ADDRESS_TO_STR_H_
