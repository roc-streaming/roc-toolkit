/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/log.h"

#include "adapters.h"

#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"

using namespace roc;

namespace {

void log_handler_adapter(const core::LogMessage& msg, void** args) {
    roc_log_handler handler_func = (roc_log_handler)args[0];
    void* handler_arg = args[1];

    roc_log_message handler_msg;
    memset(&handler_msg, 0, sizeof(handler_msg));
    api::log_message_to_user(handler_msg, msg);

    handler_func(&handler_msg, handler_arg);
}

} // namespace

void roc_log_set_level(roc_log_level level) {
    core::Logger::instance().set_level(api::log_level_from_user(level));
}

void roc_log_set_handler(roc_log_handler handler, void* argument) {
    if (handler != NULL) {
        void* args[2];
        args[0] = reinterpret_cast<void*>(handler);
        args[1] = argument;

        core::Logger::instance().set_handler(&log_handler_adapter, args,
                                             ROC_ARRAY_SIZE(args));
    } else {
        core::Logger::instance().set_handler(NULL, NULL, 0);
    }
}
