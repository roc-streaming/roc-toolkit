/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/context.h"

#include "config_helpers.h"

#include "roc_core/log.h"
#include "roc_peer/context.h"

using namespace roc;

roc_context* roc_context_open(const roc_context_config* config) {
    roc_log(LogInfo, "roc_context_open: opening context");

    if (!config) {
        roc_log(LogError, "roc_context_open: invalid arguments: config is null");
        return NULL;
    }

    peer::ContextConfig imp_config;
    if (!api::make_context_config(imp_config, *config)) {
        roc_log(LogError, "roc_context_open: invalid arguments: bad config");
        return NULL;
    }

    peer::Context* imp_context = new (std::nothrow) peer::Context(imp_config);
    if (!imp_context) {
        roc_log(LogError, "roc_context_open: can't allocate context");
        return NULL;
    }

    if (!imp_context->valid()) {
        roc_log(LogError, "roc_context_open: can't initialize context");

        delete imp_context;
        return NULL;
    }

    return (roc_context*)imp_context;
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

    delete imp_context;

    roc_log(LogInfo, "roc_context_close: closed context");

    return 0;
}
