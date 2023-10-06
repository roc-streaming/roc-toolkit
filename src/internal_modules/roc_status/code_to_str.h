/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_status/code_to_str.h
//! @brief Format status code to string.

#ifndef ROC_STATUS_CODE_TO_STR_H_
#define ROC_STATUS_CODE_TO_STR_H_

#include "roc_status/status_code.h"

namespace roc {
namespace status {

//! Get human-readable message for status code.
const char* code_to_str(StatusCode code);

} // namespace status
} // namespace roc

#endif // ROC_STATUS_CODE_TO_STR_H_
