/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/log.h"

#include "log_helpers.h"

#include "roc_core/log.h"

using namespace roc;

void roc_log_set_level(roc_log_level level) {
    core::Logger::instance().set_level(api::convert_log_level(level));
}

void roc_log_set_handler(roc_log_handler handler) {
    core::Logger::instance().set_handler(core::LogHandler(handler));
}
