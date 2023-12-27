/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_status/status_code.h
//! @brief Status codes.

#ifndef ROC_STATUS_STATUS_CODE_H_
#define ROC_STATUS_STATUS_CODE_H_

namespace roc {
namespace status {

//! Status code.
enum StatusCode {
    StatusOK,      //!< Status indicating a success of an operation.
    StatusUnknown, //!< Unknown status.
    StatusNoData,  //!< There is no enough data to perform an operation.
    StatusNoMem,   //!< Allocation failed during operation.
    StatusNoSpace, //!< Not enough space in buffer.
    StatusLimit    //!< Operation forbidden because limit exceeded.
};

} // namespace status
} // namespace roc

#endif // ROC_STATUS_STATUS_CODE_H_
