/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/socket_addr_to_str.h
//! @brief Convert network address to string.

#ifndef ROC_ADDRESS_SOCKET_ADDR_TO_STR_H_
#define ROC_ADDRESS_SOCKET_ADDR_TO_STR_H_

#include "roc_address/socket_addr.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace address {

//! Convert network address to string.
class socket_addr_to_str : public core::NonCopyable<> {
public:
    //! Construct.
    explicit socket_addr_to_str(const SocketAddr&);

    //! Get formatted address.
    const char* c_str() const {
        return buffer_;
    }

private:
    bool format_(const SocketAddr&);

    char buffer_[SocketAddr::MaxStrLen];
};

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_SOCKET_ADDR_TO_STR_H_
