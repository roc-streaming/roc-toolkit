/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_LOG_HELPERS_H_
#define ROC_LOG_HELPERS_H_

#include "roc/log.h"

#include "roc_core/log.h"

namespace roc {
namespace api {

LogLevel convert_log_level(roc_log_level level);

} // namespace api
} // namespace roc

#endif // ROC_LOG_HELPERS_H_
