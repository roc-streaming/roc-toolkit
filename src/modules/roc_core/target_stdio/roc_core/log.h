/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_stdio/roc_core/log.h
//! @brief Logging.

#ifndef ROC_CORE_LOG_H_
#define ROC_CORE_LOG_H_

#include "roc_core/attributes.h"
#include "roc_core/stddefs.h"
#include "roc_core/helpers.h"

#ifndef ROC_MODULE
#define ROC_MODULE roc //!< Default module name.
#endif

//! Print message to log.
#define roc_log(...) ::roc::core::print_message(ROC_STRINGIZE(ROC_MODULE), __VA_ARGS__)

namespace roc {

//! Log level.
enum LogLevel {
    LogNone,  //!< Disable all messages.
    LogError, //!< Error message.
    LogInfo,  //!< Informational message.
    LogDebug, //!< Debug message.
    LogTrace  //!< Debug message (extra verbosity).
};

namespace core {

//! Log handler.
typedef void (*LogHandler)(LogLevel level, const char* module, const char* message);

//! Print message to log.
void print_message(const char* module, LogLevel level, const char* format, ...)
    ROC_ATTR_PRINTF(3, 4);

//! Get current maximum log level.
LogLevel get_log_level();

//! Set maximum log level.
//!
//! @remarks
//!  Messages with higher log level will be dropped.
//!
//! @returns
//!  previous log level.
//!
//! @note
//!  Default log level is LogError.
LogLevel set_log_level(LogLevel);

//! Set log handler.
//!
//! @remarks
//!  If @p handler is not NULL, messages will be passed to @p handler
//!  instead of printing to stderr.
//!
//! @returns
//!  previous log handler.
//!
//! @note
//!  Default log handler is NULL, so that messages are printed to
//!  stderr by default.
LogHandler set_log_handler(LogHandler handler);

} // namespace core
} // namespace roc

#endif // ROC_CORE_LOG_H_
