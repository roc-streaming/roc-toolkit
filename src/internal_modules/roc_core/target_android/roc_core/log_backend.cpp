/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log_backend.h"
#include "roc_core/log.h"

#include <android/log.h>

namespace roc {
namespace core {

namespace {

int level_to_android(LogLevel level) {
    switch (level) {
    case LogError:
        return ANDROID_LOG_ERROR;

    case LogInfo:
        return ANDROID_LOG_INFO;

    case LogNote:
    case LogDebug:
        return ANDROID_LOG_DEBUG;

    case LogTrace:
    case LogNone:
        break;
    }

    return ANDROID_LOG_VERBOSE;
}

} // namespace

LogBackend::LogBackend() {
}

void LogBackend::handle(const LogMessage& msg) {
    __android_log_print(level_to_android(msg.level), "roc", "%s: %s", msg.module,
                        msg.text);
}

} // namespace core
} // namespace roc
