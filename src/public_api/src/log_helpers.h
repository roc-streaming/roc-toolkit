/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PUBLIC_API_LOG_HELPERS_H_
#define ROC_PUBLIC_API_LOG_HELPERS_H_

#include "roc/log.h"

#include "roc_core/log.h"

namespace roc {
namespace api {

LogLevel log_level_from_user(roc_log_level level);
roc_log_level log_level_to_user(LogLevel level);

void log_message_to_user(const core::LogMessage& in, roc_log_message& out);

} // namespace api
} // namespace roc

#endif // ROC_PUBLIC_API_LOG_HELPERS_H_
