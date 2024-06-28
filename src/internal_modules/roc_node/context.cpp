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
    , packet_pool_("packet_pool", arena_)
    , packet_buffer_pool_(
          "packet_buffer_pool", arena_, sizeof(core::Buffer) + config.max_packet_size)
    , frame_pool_("frame_pool", arena_)
    , frame_buffer_pool_(
          "frame_buffer_pool", arena_, sizeof(core::Buffer) + config.max_frame_size)
    , processor_map_(arena_)
    , encoding_map_(arena_)
    , network_loop_(packet_pool_, packet_buffer_pool_, arena_)
    , control_loop_(network_loop_, arena_)
    , init_status_(status::NoStatus) {
    roc_log(LogDebug, "context: initializing");

    if ((init_status_ = network_loop_.init_status()) != status::StatusOK) {
        roc_log(LogError, "context: can't create network loop: status=%s",
                status::code_to_str(init_status_));
        return;
    }

    if ((init_status_ = control_loop_.init_status()) != status::StatusOK) {
        roc_log(LogError, "context: can't create control loop: status=%s",
                status::code_to_str(init_status_));
        return;
    }

    init_status_ = status::StatusOK;
}

Context::~Context() {
    roc_log(LogDebug, "context: deinitializing");
}

status::StatusCode Context::init_status() const {
    return init_status_;
}

core::IArena& Context::arena() {
    return arena_;
}

core::IPool& Context::packet_pool() {
    return packet_pool_;
}

core::IPool& Context::packet_buffer_pool() {
    return packet_buffer_pool_;
}

core::IPool& Context::frame_pool() {
    return frame_pool_;
}

core::IPool& Context::frame_buffer_pool() {
    return frame_buffer_pool_;
}

audio::ProcessorMap& Context::processor_map() {
    return processor_map_;
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
