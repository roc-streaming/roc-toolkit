/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_address/network_uri_to_str.h
//! @brief Format NetworkUri to string.

#ifndef ROC_ADDRESS_NETWORK_URI_TO_STR_H_
#define ROC_ADDRESS_NETWORK_URI_TO_STR_H_

#include "roc_address/network_uri.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace address {

//! Convert NetworkUri to string.
class network_uri_to_str : public core::NonCopyable<> {
public:
    //! Construct.
    explicit network_uri_to_str(const NetworkUri&);

    //! Get formatted string.
    const char* c_str() const {
        return buf_;
    }

private:
    char buf_[1024];
};

} // namespace address
} // namespace roc

#endif // ROC_ADDRESS_NETWORK_URI_TO_STR_H_
