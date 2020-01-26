/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/context.h"

#include "config_helpers.h"
#include "root_allocator.h"

#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_peer/context.h"

using namespace roc;

int roc_context_open(const roc_context_config* config, roc_context** result) {
    roc_log(LogInfo, "roc_context_open: opening context");

    if (!result) {
        roc_log(LogError, "roc_context_open: invalid arguments: result is null");
        return -1;
    }

    if (!config) {
        roc_log(LogError, "roc_context_open: invalid arguments: config is null");
        return -1;
    }

    peer::ContextConfig imp_config;
    if (!api::make_context_config(imp_config, *config)) {
        roc_log(LogError, "roc_context_open: invalid arguments: bad config");
        return -1;
    }

    core::ScopedPtr<peer::Context> imp_context(
        new (api::root_allocator) peer::Context(imp_config, api::root_allocator),
        api::root_allocator);

    if (!imp_context) {
        roc_log(LogError, "roc_context_open: can't allocate context");
        return -1;
    }

    if (!imp_context->valid()) {
        roc_log(LogError, "roc_context_open: can't initialize context");
        return -1;
    }

    *result = (roc_context*)imp_context.release();
    return 0;
}

int roc_context_close(roc_context* context) {
    if (!context) {
        roc_log(LogError, "roc_context_close: invalid arguments: context is null");
        return -1;
    }

    peer::Context* imp_context = (peer::Context*)context;

    if (imp_context->is_used()) {
        roc_log(LogError, "roc_context_close: context is still in use");
        return -1;
    }

    imp_context->destroy();

    roc_log(LogInfo, "roc_context_close: closed context");

    return 0;
}
