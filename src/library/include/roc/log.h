/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * @file roc/log.h
 * @brief Logger configuration.
 */

#ifndef ROC_LOG_H_
#define ROC_LOG_H_

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Log level.
 * @see roc_log_set_level
 */
typedef enum roc_log_level {
    /** No messages.
     * Setting this level disables logging completely.
     */
    ROC_LOG_NONE = 0,

    /** Error messages.
     * Setting this level enables logging only when something goes wrong, e.g. a user
     * operation can't be completed, or there is not enough memory for a new session.
     */
    ROC_LOG_ERROR = 1,

    /** Informational messages.
     * Setting this level enables logging of important high-level events, like binding
     * a new port or creating a new session.
     */
    ROC_LOG_INFO = 2,

    /** Debug messages.
     * Setting this level enables logging of debug messages. Doesn't affect performance.
     */
    ROC_LOG_DEBUG = 3,

    /** Debug messages (extra verbosity).
     * Setting this level enables verbose tracing. May cause significant slow down.
     */
    ROC_LOG_TRACE = 4
} roc_log_level;

/** Log handler.
 *
 * @b Parameters
 *  - @p level defines the message level
 *  - @p component defines the component that produces the message
 *  - @p message defines the message text
 *
 * @see roc_log_set_handler
 */
typedef void (*roc_log_handler)(roc_log_level level,
                                const char* component,
                                const char* message);

/** Set maximum log level.
 *
 * Messages with log levels higher than @p level will be dropped.
 * By default the log level is set to @c ROC_LOG_ERROR.
 *
 * @b Thread-safety
 *  - can be used concurrently
 */
ROC_API void roc_log_set_level(roc_log_level level);

/** Set log handler.
 *
 * If @p handler is not NULL, messages are passed to the handler. Otherwise, messages are
 * printed to stderr. By default the log handler is set to NULL.
 *
 * It's guaranteed that the previously set handler, if any, will not be used after this
 * function returns.
 *
 * @b Thread-safety
 *  - can be used concurrently
 *  - handler calls are serialized, so the handler itself doesn't need to be thread-safe
 */
ROC_API void roc_log_set_handler(roc_log_handler handler);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_LOG_H_ */
