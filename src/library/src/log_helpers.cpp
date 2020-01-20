/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "log_helpers.h"

namespace roc {
namespace api {

LogLevel convert_log_level(roc_log_level level) {
    switch ((unsigned)level) {
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

    return core::DefaultLogLevel;
}

} // namespace api
} // namespace roc
