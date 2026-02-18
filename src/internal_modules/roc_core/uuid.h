/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/uuid.h
//! @brief UUID generation.

#ifndef ROC_CORE_UUID_H_
#define ROC_CORE_UUID_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

enum {
    //! Number of characters in UUID string.
    UuidLen = 36
};

//! Generate UUID string into given buffer.
//! @note
//!  Generated string has UuidLen characters + null terminator.
//!  Panics if @p buf is null or @p buf_sz is less than UuidLen + 1.
bool uuid_generate(char* buf, size_t buf_sz);

} // namespace core
} // namespace roc

#endif // ROC_CORE_UUID_H_
