/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_posix/roc_core/errno_to_str.h
//! @brief Convert errno to string.

#ifndef ROC_CORE_ERRNO_TO_STR_H_
#define ROC_CORE_ERRNO_TO_STR_H_

#include "roc_core/string_buffer.h"

namespace roc {
namespace core {

//! Convert errno to string.
//! @remarks
//!  Uses strerror_r(), which thread-safe unlike strerror().
class errno_to_str : public StringBuffer<96> {
public:
    //! Construct from errno.
    errno_to_str();

    //! Construct from custom error code.
    explicit errno_to_str(int err);
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_ERRNO_TO_STR_H_
