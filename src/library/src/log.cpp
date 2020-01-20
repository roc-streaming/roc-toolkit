/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/log.h"

#include "roc_core/log.h"

using namespace roc;

namespace {

roc::LogLevel convert_level(roc_log_level level) {
    switch ((unsigned)level) {
    case ROC_LOG_NONE:
        return roc::LogNone;

    case ROC_LOG_ERROR:
        return roc::LogError;

    case ROC_LOG_INFO:
        return roc::LogInfo;

    case ROC_LOG_DEBUG:
        return roc::LogDebug;

    case ROC_LOG_TRACE:
        return roc::LogTrace;

    default:
        break;
    }

    return core::DefaultLogLevel;
}

} // namespace

void roc_log_set_level(roc_log_level level) {
    core::Logger::instance().set_level(convert_level(level));
}

void roc_log_set_handler(roc_log_handler handler) {
    core::Logger::instance().set_handler(core::LogHandler(handler));
}
