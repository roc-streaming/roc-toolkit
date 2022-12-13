/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "log_helpers.h"

namespace roc {
namespace api {

LogLevel log_level_from_user(roc_log_level level) {
    switch (level) {
    case ROC_LOG_NONE:
        return LogNone;

    case ROC_LOG_ERROR:
        return LogError;

    case ROC_LOG_INFO:
        return LogInfo;

    case ROC_LOG_DEBUG:
        return LogDebug;

    case ROC_LOG_TRACE:
        return LogTrace;

    default:
        break;
    }

    return LogError;
}

roc_log_level log_level_to_user(LogLevel level) {
    switch (level) {
    case LogNone:
        return ROC_LOG_NONE;

    case LogError:
        return ROC_LOG_ERROR;

    case LogInfo:
        return ROC_LOG_INFO;

    case LogDebug:
        return ROC_LOG_DEBUG;

    case LogTrace:
        return ROC_LOG_TRACE;

    default:
        break;
    }

    return ROC_LOG_ERROR;
}

void log_message_to_user(const core::LogMessage& in, roc_log_message& out) {
    out.level = log_level_to_user(in.level);
    out.module = in.module;
    out.file = in.file;
    out.line = in.line;
    out.time = (unsigned long long)in.time;
    out.pid = (unsigned long long)in.pid;
    out.tid = (unsigned long long)in.tid;
    out.text = in.message;
}

} // namespace api
} // namespace roc
