/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/target_pc/roc_core/log.h
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
            logger.writef((level), ROC_STRINGIZE(ROC_MODULE), __FILE__, __LINE__,        \
                          __VA_ARGS__);                                                  \
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

//! Colors mode.
enum ColorsMode {
    ColorsDisabled, //!< Do not use colored logs.
    ColorsEnabled   //!< Use colored logs.
};

//! Location mode.
enum LocationMode {
    LocationDisabled, //!< Do not show location.
    LocationEnabled   //!< Show location.
};

//! Log handler.
typedef void (*LogHandler)(
    LogLevel level, const char* module, const char* file, int line, const char* message);

//! Logger.
class Logger : public NonCopyable<> {
public:
    //! Get logger instance.
    static Logger& instance() {
        return Singleton<Logger>::instance();
    }

    //! Print message to log.
    ROC_ATTR_PRINTF(6, 7)
    void writef(LogLevel level,
                const char* module,
                const char* file,
                int line,
                const char* format,
                ...);

    //! Get current maximum log level.
    LogLevel get_level() const {
        return (LogLevel)AtomicOps::load_relaxed(level_);
    }

    //! Set verbosity level.
    //! @remarks
    //!  Sets level and location according to requested verbosity level.
    void set_verbosity(unsigned);

    //! Set maximum log level.
    //! @remarks
    //!  Messages with higher log level will be dropped.
    //! @note
    //!  Other threads are not guaranteed to see the change immediately.
    void set_level(LogLevel);

    //! Set location mode.
    //! @note
    //!  Other threads will see the change immediately.
    void set_location(LocationMode);

    //! Set colors mode.
    //! @note
    //!  Other threads will see the change immediately.
    void set_colors(ColorsMode);

    //! Set log handler.
    //! @remarks
    //!  If @p handler is not NULL, log messages will be passed to @p handler.
    //!  Otherwise, they're printed to stderr. Default log handler is NULL.
    //! @note
    //!  Other threads will see the change immediately.
    void set_handler(LogHandler handler);

private:
    friend class Singleton<Logger>;

    Logger();

    void default_print_(LogLevel level,
                        const char* module,
                        const char* file,
                        int line,
                        const char* message);

    int level_;

    Mutex mutex_;

    LogHandler handler_;
    ColorsMode colors_;
    LocationMode location_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LOG_H_
