/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "private.h"

#include "roc_core/log.h"

using namespace roc;

roc_context::roc_context(const roc_context_config& cfg)
    : packet_pool(allocator, false)
    , byte_buffer_pool(allocator, cfg.max_packet_size, false)
    , sample_buffer_pool(allocator, cfg.max_frame_size / sizeof(audio::sample_t), false)
    , event_loop(packet_pool, byte_buffer_pool, allocator)
    , counter(0) {
}

roc_context* roc_context_open(const roc_context_config* config) {
    roc_log(LogInfo, "roc_context: opening context");

    if (!config) {
        roc_log(LogError, "roc_context_open: invalid arguments: config is null");
        return NULL;
    }

    roc_context_config private_config;
    if (!make_context_config(private_config, *config)) {
        roc_log(LogError, "roc_context_open: invalid arguments: bad config");
        return NULL;
    }

    roc_context* context = new (std::nothrow) roc_context(private_config);
    if (!context) {
        roc_log(LogError, "roc_context_open: can't allocate roc_context");
        return NULL;
    }

    if (!context->event_loop.valid()) {
        roc_log(LogError, "roc_context_open: can't initialize transceiver");

        delete context;
        return NULL;
    }

    return context;
}

int roc_context_close(roc_context* context) {
    if (!context) {
        roc_log(LogError, "roc_context_close: invalid arguments: context is null");
        return -1;
    }

    if (context->counter != 0) {
        roc_log(LogError, "roc_context_close: context is still in use: counter=%lu",
                (unsigned long)context->counter);
        return -1;
    }

    delete context;

    roc_log(LogInfo, "roc_context: closed context");

    return 0;
}
