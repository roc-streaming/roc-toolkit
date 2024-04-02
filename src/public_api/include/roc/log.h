/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

/**
 * \file roc/log.h
 * \brief Logging.
 */

#ifndef ROC_LOG_H_
#define ROC_LOG_H_

#include "roc/platform.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Log level.
 * \see roc_log_set_level
 */
typedef enum roc_log_level {
    /** No messages.
     * Setting this level disables logging completely.
     */
    ROC_LOG_NONE = 0,

    /** Error messages.
     * Setting this level enables logging only when something goes wrong, e.g. a user
     * operation can't be completed, or there is not enough memory.
     */
    ROC_LOG_ERROR = 1,

    /** Informational messages.
     * Setting this level enables logging of important high-level events, like binding
     * a new port or accepting a new connection.
     */
    ROC_LOG_INFO = 2,

    /** Noteworthy debug messages.
     * Setting this level enables logging of debug messages, but only those which
     * are generated for more rare and important events, like changing latency.
     */
    ROC_LOG_NOTE = 3,

    /** Debug messages.
     * Setting this level enables logging of debug messages.
     */
    ROC_LOG_DEBUG = 4,

    /** Extra verbose debug messages;
     * Setting this level enables verbose tracing.
     * Unlike all other levels, may cause significant slow down.
     */
    ROC_LOG_TRACE = 5
} roc_log_level;

/** Log message.
 * \see roc_log_set_handler
 */
typedef struct roc_log_message {
    /** Message log level.
     */
    roc_log_level level;

    /** Name of the module that originated the message.
     * Pointer can be used only until roc_log_handler() returns.
     */
    const char* module;

    /** Name of the source code file.
     * May be NULL.
     * Pointer can be used only until roc_log_handler() returns.
     */
    const char* file;

    /** Line number in the source code file.
     */
    int line;

    /** Message timestamp, nanoseconds since Unix epoch.
     */
    unsigned long long time;

    /** Platform-specific process ID.
     */
    unsigned long long pid;

    /** Platform-specific thread ID.
     */
    unsigned long long tid;

    /** Message text.
     * Pointer can be used only until roc_log_handler() returns.
     */
    const char* text;
} roc_log_message;

/** Log handler.
 *
 * **Parameters**
 *  - \p message define message to be logged
 *  - \p argument is the argument passed to roc_log_set_handler
 *
 * \see roc_log_set_handler
 */
typedef void (*roc_log_handler)(const roc_log_message* message, void* argument);

/** Set maximum log level.
 *
 * Messages with log levels higher than \p level will be dropped.
 * By default the log level is set to \ref ROC_LOG_ERROR.
 *
 * **Thread safety**
 *
 * Can be used concurrently.
 */
ROC_API void roc_log_set_level(roc_log_level level);

/** Set log handler.
 *
 * If \p handler is not NULL, messages are passed to the handler. Otherwise, messages are
 * printed to stderr. By default the log handler is set to NULL.
 *
 * \p argument will be passed to the handler each time it is invoked.
 *
 * It's guaranteed that the previously set handler, if any, will not be used after this
 * function returns.
 *
 * **Thread safety**
 *
 * Can be used concurrently. Handler calls are serialized, so the handler itself doesn't
 * need to be thread-safe.
 */
ROC_API void roc_log_set_handler(roc_log_handler handler, void* argument);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ROC_LOG_H_ */
