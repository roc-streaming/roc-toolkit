/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @brief Roc log.

#ifndef ROC_LOG_H_
#define ROC_LOG_H_

#include "roc/types.h"

#ifdef __cplusplus
extern "C" {
#endif

//! Log level.
typedef enum roc_log_level {
    ROC_LOG_NONE,  //!< Disable all messages.
    ROC_LOG_ERROR, //!< Error messages.
    ROC_LOG_INFO,  //!< Informational messages.
    ROC_LOG_DEBUG, //!< Debug messages.
    ROC_LOG_TRACE  //!< Debug messages (extra verbosity).
} roc_log_level;

//! Log handler.
typedef void (*roc_log_handler)(roc_log_level level,
                                const char* module,
                                const char* message);

//! Set maximum log level.
//! Messages with higher log level will be dropped.
//! Default log level is ROC_LOG_ERROR.
ROC_API void roc_log_set_level(roc_log_level level);

//! Set log handler.
//! If @p handler is not NULL, messages will be passed to @p handler
//! instead of printing to stderr. Default log handler is NULL, so
//! messages are printed to stderr by default.
ROC_API void roc_log_set_handler(roc_log_handler handler);

#ifdef __cplusplus
}
#endif

#endif // ROC_LOG_H_
