/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/cname.h
//! @brief CNAME utilities.

#ifndef ROC_RTCP_CNAME_H_
#define ROC_RTCP_CNAME_H_

#include "roc_core/noncopyable.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

//! Maximum allowed CNAME length.
static const size_t MaxCnameLen = header::MaxTextLen;

//! Get printable representation of CNAME.
class cname_to_str : public core::NonCopyable<> {
public:
    //! Construct from custom error code.
    explicit cname_to_str(const char* cname);

    //! Get error message.
    const char* c_str() const {
        return buffer_;
    }

private:
    char buffer_[MaxCnameLen * 3 + 4];
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_CNAME_H_
