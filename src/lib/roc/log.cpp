/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/log.h"

#include "roc_core/log.h"

using namespace roc;

void roc_log_set_level(roc_log_level level) {
    core::set_log_level(roc::LogLevel(level));
}

void roc_log_set_handler(roc_log_handler handler) {
    core::set_log_handler(core::LogHandler(handler));
}
