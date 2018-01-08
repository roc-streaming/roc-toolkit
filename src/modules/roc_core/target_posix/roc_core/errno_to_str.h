/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/errno_to_str.h
//! @brief Convert errno to string.

#ifndef ROC_CORE_ERRNO_TO_STR_H_
#define ROC_CORE_ERRNO_TO_STR_H_

#include "roc_core/noncopyable.h"

namespace roc {
namespace core {

//! Convert errno to string.
//! @remarks
//!  Uses strerror_r(), which is thread-safe unlike strerror().
class errno_to_str : public NonCopyable<> {
public:
    //! Construct from errno.
    errno_to_str();

    //! Construct from custom error code.
    explicit errno_to_str(int err);

    //! Get error message.
    const char* c_str() const {
        return buffer_;
    }

private:
    char buffer_[96];
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ERRNO_TO_STR_H_
