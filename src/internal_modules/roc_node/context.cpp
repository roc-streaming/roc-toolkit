/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_node/context.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace node {

Context::Context(const ContextConfig& config, core::IArena& arena)
    : arena_(arena)
    , packet_factory_(arena_)
    , byte_buffer_factory_(arena_, config.max_packet_size)
    , sample_buffer_factory_(arena_, config.max_frame_size / sizeof(audio::sample_t))
    , encoding_map_(arena_)
    , network_loop_(packet_factory_, byte_buffer_factory_, arena_)
    , control_loop_(network_loop_, arena_) {
    roc_log(LogDebug, "context: initializing");
}

Context::~Context() {
    roc_log(LogDebug, "context: deinitializing");
}

bool Context::is_valid() {
    return network_loop_.is_valid() && control_loop_.is_valid();
}

core::IArena& Context::arena() {
    return arena_;
}

packet::PacketFactory& Context::packet_factory() {
    return packet_factory_;
}

core::BufferFactory<uint8_t>& Context::byte_buffer_factory() {
    return byte_buffer_factory_;
}

core::BufferFactory<audio::sample_t>& Context::sample_buffer_factory() {
    return sample_buffer_factory_;
}

rtp::EncodingMap& Context::encoding_map() {
    return encoding_map_;
}

netio::NetworkLoop& Context::network_loop() {
    return network_loop_;
}

ctl::ControlLoop& Context::control_loop() {
    return control_loop_;
}

} // namespace node
} // namespace roc
