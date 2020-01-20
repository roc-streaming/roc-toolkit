/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/context.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace peer {

Context::Context(const ContextConfig& config)
    : packet_pool_(allocator_, false)
    , byte_buffer_pool_(allocator_, config.max_packet_size, config.poisoning)
    , sample_buffer_pool_(
          allocator_, config.max_frame_size / sizeof(audio::sample_t), config.poisoning)
    , event_loop_(packet_pool_, byte_buffer_pool_, allocator_)
    , ref_counter_(0) {
    roc_log(LogDebug, "context: initializing");
}

Context::~Context() {
    roc_log(LogDebug, "context: deinitializing");

    if (is_used()) {
        roc_panic("context: still in use when destroying: refcounter=%u",
                  (unsigned)ref_counter_);
    }
}

bool Context::valid() {
    return event_loop_.valid();
}

void Context::incref() {
    if (!valid()) {
        roc_panic("context: can't use invalid context");
    }
    ++ref_counter_;
}

void Context::decref() {
    if (!valid()) {
        roc_panic("context: can't use invalid context");
    }
    --ref_counter_;
}

bool Context::is_used() {
    return ref_counter_ != 0;
}

core::IAllocator& Context::allocator() {
    return allocator_;
}

packet::PacketPool& Context::packet_pool() {
    return packet_pool_;
}

core::BufferPool<uint8_t>& Context::byte_buffer_pool() {
    return byte_buffer_pool_;
}

core::BufferPool<audio::sample_t>& Context::sample_buffer_pool() {
    return sample_buffer_pool_;
}

netio::EventLoop& Context::event_loop() {
    return event_loop_;
}

} // namespace peer
} // namespace roc
