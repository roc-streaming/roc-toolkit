/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_core/thread.h"

namespace roc {
namespace core {

namespace {

void backend_handler(const LogMessage& msg, void** args) {
    roc_panic_if(!args);
    roc_panic_if(!args[0]);

    ((LogBackend*)args[0])->handle(msg);
}

struct logger_destructor {
    static int flag;

    static bool destructor_called() {
        return AtomicOps::load_relaxed(flag);
    }

    ~logger_destructor() {
        AtomicOps::store_seq_cst(flag, true);
    }
};

int logger_destructor::flag = 0;
logger_destructor logger_dtor;

} // namespace

Logger::Logger()
    : level_(LogError)
    , colors_mode_(ColorsDisabled)
    , location_mode_(LocationDisabled) {
    handler_ = &backend_handler;
    handler_args_[0] = &backend_;
}

void Logger::set_verbosity(unsigned verb) {
    switch (verb) {
    case 0:
        set_level(LogError);
        break;

    case 1:
        set_level(LogInfo);
        break;

    case 2:
        set_level(LogDebug);
        break;

    default:
        set_level(LogTrace);
        break;
    }
}

void Logger::set_level(LogLevel level) {
    Mutex::Lock lock(mutex_);

    if ((int)level < LogNone) {
        level = LogNone;
    }

    if ((int)level > LogTrace) {
        level = LogTrace;
    }

    if ((int)level >= LogTrace) {
        location_mode_ = LocationEnabled;
    } else {
        location_mode_ = LocationDisabled;
    }

    AtomicOps::store_relaxed(level_, level);
}

void Logger::set_colors(ColorsMode mode) {
    Mutex::Lock lock(mutex_);

    colors_mode_ = mode;
}

void Logger::set_handler(LogHandler handler, void** args, size_t n_args) {
    Mutex::Lock lock(mutex_);

    roc_panic_if(n_args > MaxArgs);
    roc_panic_if(!args && n_args);
    roc_panic_if(args && !n_args);

    if (handler) {
        handler_ = handler;
        memset(handler_args_, 0, sizeof(handler_args_));
        if (n_args) {
            memcpy(handler_args_, args, sizeof(void*) * n_args);
        }
    } else {
        handler_ = &backend_handler;
        handler_args_[0] = &backend_;
    }
}

void Logger::writef(LogLevel level,
                    const char* module,
                    const char* file,
                    int line,
                    const char* format,
                    ...) {
    Mutex::Lock lock(mutex_);

    if (level > level_ || level == LogNone) {
        return;
    }

    // If user installed custom log handler and did not uninstall it until process
    // exit, it may happen that user's library will deinitialize before our
    // library (if we're in different shared libraries). If this happened, attempt
    // to invoke handler at this point may cause crashes. To reduce probability of
    // this, we stop using user handler as soon as we have detected it.
    if (handler_ != &backend_handler && logger_destructor::destructor_called()) {
        return;
    }

    char text[512] = {};
    va_list args;
    va_start(args, format);
    if (vsnprintf(text, sizeof(text) - 1, format, args) < 0) {
        text[0] = '\0';
    }
    va_end(args);

    LogMessage msg;
    msg.level = level;
    msg.module = module;
    msg.file = file;
    msg.line = line;
    msg.time = timestamp(ClockUnix);
    msg.pid = Thread::get_pid();
    msg.tid = Thread::get_tid();
    msg.text = text;
    msg.location_mode = location_mode_;
    msg.colors_mode = colors_mode_;

    handler_(msg, handler_args_);
}

} // namespace core
} // namespace roc
