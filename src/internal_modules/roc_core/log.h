/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/log.h
//! @brief Logging.

#ifndef ROC_CORE_LOG_H_
#define ROC_CORE_LOG_H_

#include "roc_core/atomic_ops.h"
#include "roc_core/attributes.h"
#include "roc_core/log_backend.h"
#include "roc_core/mutex.h"
#include "roc_core/noncopyable.h"
#include "roc_core/singleton.h"
#include "roc_core/time.h"

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
    LogNote,  //!< Noteworthy debug message.
    LogDebug, //!< Regular debug message.
    LogTrace  //!< Extra verbose debug message.
};

namespace core {

//! Colors mode.
enum ColorsMode {
    ColorsAuto,     //!< Automatically use colored logs if colors are supported.
    ColorsEnabled,  //!< Use colored logs.
    ColorsDisabled, //!< Do not use colored logs.
};

//! Location mode.
enum LocationMode {
    LocationEnabled, //!< Show location.
    LocationDisabled //!< Do not show location.
};

//! Log message.
struct LogMessage {
    LogLevel level; //!< Logging level.

    const char* module; //!< Name of module that originated message.
    const char* file;   //!< File path.
    int line;           //!< Line number.

    nanoseconds_t time; //!< Timestamp, nanoseconds since Unix epoch.
    uint64_t pid;       //!< Plaform-specific process ID.
    uint64_t tid;       //!< Plaform-specific thread ID.

    const char* text; //!< Message text.

    LocationMode location_mode; //!< Whether to enable location.
    ColorsMode colors_mode;     //!< Whether to enable colors.

    LogMessage()
        : level(LogNone)
        , module(NULL)
        , file(NULL)
        , line(0)
        , time(0)
        , pid(0)
        , tid(0)
        , text(NULL)
        , location_mode(LocationDisabled)
        , colors_mode(ColorsDisabled) {
    }
};

//! Log handler.
typedef void (*LogHandler)(const LogMessage& message, void** args);

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
    //!  Sets logging level according to requested verbosity level.
    void set_verbosity(unsigned);

    //! Set maximum log level.
    //! @remarks
    //!  Messages with higher log level will be dropped.
    //! @note
    //!  Other threads are not guaranteed to see the change immediately.
    void set_level(LogLevel);

    //! Set colors mode.
    //! @note
    //!  Other threads will see the change immediately.
    void set_colors(ColorsMode);

    //! Set log handler.
    //! @remarks
    //!  If @p handler is not NULL, log messages and @p arg will be passed to
    //!  @p handler. Otherwise, they're printed to stderr.
    //! @note
    //!  Other threads will see the change immediately.
    void set_handler(LogHandler handler, void** args, size_t n_args);

private:
    friend class Singleton<Logger>;

    enum { MaxArgs = 8 };

    Logger();

    int level_;

    Mutex mutex_;

    LogHandler handler_;
    void* handler_args_[MaxArgs];

    LogBackend backend_;

    ColorsMode colors_mode_;
    LocationMode location_mode_;
};

} // namespace core
} // namespace roc

#endif // ROC_CORE_LOG_H_
