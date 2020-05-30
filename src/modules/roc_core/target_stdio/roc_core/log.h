/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_stdio/roc_core/log.h
//! @brief Logging.

#ifndef ROC_CORE_LOG_H_
#define ROC_CORE_LOG_H_

#include "roc_core/atomic_ops.h"
#include "roc_core/attributes.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"

#ifndef ROC_MODULE
#error "ROC_MODULE not defined"
#endif

//! Print message to log.
//! @remarks
//!  If the given log level is disabled, this call does not insert memory barriers
//!  and does not evaluate arguments except @p level.
#define roc_log(level, ...)                                                              \
    do {                                                                                 \
        ::roc::core::Logger& logger = ::roc::core::Logger::instance();                   \
        if ((level) <= logger.get_level()) {                                             \
            logger.print(ROC_STRINGIZE(ROC_MODULE), (level), __VA_ARGS__);               \
        }                                                                                \
    } while (0)

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

//! Default log level.
const LogLevel DefaultLogLevel = LogError;

//! Colors mode.
enum ColorsMode {
    ColorsDisabled, //!< Do not use colored logs.
    ColorsEnabled   //!< Use colored logs.
};

//! Default colors mode.
const ColorsMode DefaultColorsMode = ColorsDisabled;

//! Log handler.
typedef void (*LogHandler)(LogLevel level, const char* module, const char* message);

//! Logger.
class Logger : public NonCopyable<> {
public:
    //! Get logger instance.
    static Logger& instance() {
        return Singleton<Logger>::instance();
    }

    //! Print message to log.
    void print(const char* module, LogLevel level, const char* format, ...)
        ROC_ATTR_PRINTF(4, 5);

    //! Get current maximum log level.
    LogLevel get_level() {
        return (LogLevel)AtomicOps::load_relaxed(level_);
    }

    //! Set maximum log level.
    //! @remarks
    //!  Messages with higher log level will be dropped.
    //! @note
    //!  Other threads are not guaranteed to see the change immediately.
    void set_level(LogLevel);

    //! Set log handler.
    //! @remarks
    //!  If @p handler is not NULL, log messages will be passed to @p handler.
    //!  Otherwise, they're printed to stderr. Default log handler is NULL.
    //! @note
    //!  Other threads will see the change immediately.
    void set_handler(LogHandler handler);

    //! Set colors mode.
    //! @note
    //!  Other threads will see the change immediately.
    void set_colors(ColorsMode mode);

private:
    friend class Singleton<Logger>;

    Logger();

    void default_print_(LogLevel level, const char* module, const char* message);

    int level_;

    Mutex mutex_;

    LogHandler handler_;
    ColorsMode colors_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LOG_H_
