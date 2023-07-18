/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_error/error_code_to_str.h
//! @brief Format error code to string.

#ifndef ROC_ERROR_ERROR_CODE_TO_STR_H_
#define ROC_ERROR_ERROR_CODE_TO_STR_H_

#include "roc_error/error_code.h"

namespace roc {
namespace error {

//! Get human-readable message for error code.
const char* error_code_to_str(ErrorCode err);

} // namespace error
} // namespace roc

#endif // ROC_ERROR_ERROR_CODE_TO_STR_H_
