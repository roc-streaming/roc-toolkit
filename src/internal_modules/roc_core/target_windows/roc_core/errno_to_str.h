/*
 * Copyright (c) 2025 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_windows/roc_core/errno_to_str.h
//! @brief Convert errno to string.

#ifndef ROC_CORE_ERRNO_TO_STR_H_
#define ROC_CORE_ERRNO_TO_STR_H_

#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Convert errno to string.
//! @remarks
//!  Uses FormatMessageA() to convert Windows error codes to strings.
class errno_to_str : public NonCopyable<> {
public:
    //! Construct from GetLastError().
    errno_to_str();

    //! Construct from custom error code.
    explicit errno_to_str(int err);

    //! Get error message.
    const char* c_str() const {
        return buffer_;
    }

private:
    void format_(int err);

    char buffer_[256];
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ERRNO_TO_STR_H_
