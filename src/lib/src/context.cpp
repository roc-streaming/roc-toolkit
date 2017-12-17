/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "private.h"

#include "roc_core/log.h"

using namespace roc;

roc_context::roc_context(const roc_context_config& cfg)
    : packet_pool(allocator, cfg.chunk_size / cfg.max_packet_size)
    , byte_buffer_pool(
          allocator, cfg.max_packet_size, cfg.chunk_size / cfg.max_packet_size)
    , sample_buffer_pool(allocator,
                         cfg.max_frame_size / sizeof(audio::sample_t),
                         cfg.chunk_size / cfg.max_frame_size)
    , trx(packet_pool, byte_buffer_pool, allocator)
    , started(false)
    , stopped(false) {
}

roc_context* roc_context_open(const roc_context_config* config) {
    roc_log(LogInfo, "roc_context: opening context");

    roc_context_config cconfig;
    if (!config_context(cconfig, config)) {
        roc_log(LogError, "roc_context_open: invalid config");
        return NULL;
    }

    roc_context* context = new (std::nothrow) roc_context(cconfig);
    if (!context) {
        roc_log(LogError, "roc_context_open: can't allocate roc_context");
        return NULL;
    }

    return context;
}

int roc_context_start(roc_context* context) {
    if (!context) {
        roc_log(LogError, "roc_context_start: invalid arguments: context == NULL");
        return -1;
    }

    if (context->started) {
        roc_log(LogError, "roc_context_start: context is already started");
        return -1;
    }

    roc_log(LogInfo, "roc_context: starting context");

    context->trx.start();
    context->started = true;

    return 0;
}

void roc_context_stop(roc_context* context) {
    if (!context) {
        roc_log(LogError, "roc_context_stop: invalid arguments: context == NULL");
        return;
    }

    if (!context->started) {
        roc_log(LogDebug, "roc_context_stop: context is not started");
        return;
    }

    if (context->stopped) {
        roc_log(LogDebug, "roc_context_stop: context is already stopped");
        return;
    }

    context->trx.stop();
    context->trx.join();

    context->stopped = true;

    roc_log(LogInfo, "roc_context: stopped context");
}

int roc_context_close(roc_context* context) {
    if (!context) {
        roc_log(LogError, "roc_context_close: invalid arguments: context == NULL");
        return -1;
    }

    if (context->refcount != 0) {
        roc_log(LogError, "roc_context_close: context is still in use: refcount=%lu",
                (unsigned long)context->refcount);
        return -1;
    }

    if (context->started && !context->stopped) {
        roc_context_stop(context);
    }

    delete context;

    roc_log(LogInfo, "roc_context: closed context");

    return 0;
}
