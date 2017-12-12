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
#include "roc_core/panic.h"

using namespace roc;

namespace {

enum {
    DefaultMaxPacketSize = 2048,
    DefaultMaxFrameSize = 64 * 1024,
    DefaultChunkSize = 128 * 1024
};

} // namespace

roc_context::roc_context(size_t max_packet_size, size_t max_frame_size, size_t chunk_size)
    : packet_pool(allocator, chunk_size / max_packet_size)
    , byte_buffer_pool(allocator, max_packet_size, chunk_size / max_packet_size)
    , sample_buffer_pool(allocator,
                         max_frame_size / sizeof(audio::sample_t),
                         chunk_size / max_frame_size)
    , trx(packet_pool, byte_buffer_pool, allocator)
    , started(false)
    , stopped(false) {
}

roc_context* roc_context_open(const roc_context_config* config) {
    size_t max_packet_size = DefaultMaxPacketSize;
    size_t max_frame_size = DefaultMaxFrameSize;
    size_t chunk_size = DefaultChunkSize;

    if (config) {
        if (config->max_packet_size) {
            max_packet_size = config->max_packet_size;
        }
        if (config->max_frame_size) {
            max_frame_size = config->max_frame_size;
        }
        if (config->chunk_size) {
            chunk_size = config->chunk_size;
        }
    }

    return new(std::nothrow) roc_context(max_packet_size, max_frame_size, chunk_size);
}

int roc_context_start(roc_context* context) {
    roc_panic_if_not(context);

    if (context->started) {
        return -1;
    }

    context->trx.start();

    context->started = true;

    return 0;
}

void roc_context_stop(roc_context* context) {
    roc_panic_if_not(context);

    if (!context->started || context->stopped) {
        return;
    }

    context->trx.stop();
    context->trx.join();

    context->stopped = true;
}

void roc_context_close(roc_context* context) {
    roc_panic_if_not(context);

    if (context->started && !context->stopped) {
        roc_context_stop(context);
    }

    delete context;
}
