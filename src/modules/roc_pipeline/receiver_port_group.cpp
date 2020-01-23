/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_pipeline/receiver_port_group.h"
#include "roc_core/log.h"

namespace roc {
namespace pipeline {

ReceiverPortGroup::ReceiverPortGroup(
    const ReceiverConfig& receiver_config,
    ReceiverState& receiver_state,
    audio::Mixer& mixer,
    const rtp::FormatMap& format_map,
    packet::PacketPool& packet_pool,
    core::BufferPool<uint8_t>& byte_buffer_pool,
    core::BufferPool<audio::sample_t>& sample_buffer_pool,
    core::IAllocator& allocator)
    : allocator_(allocator)
    , format_map_(format_map)
    , receiver_state_(receiver_state)
    , session_group_(receiver_config,
                     receiver_state,
                     mixer,
                     format_map,
                     packet_pool,
                     byte_buffer_pool,
                     sample_buffer_pool,
                     allocator) {
}

void ReceiverPortGroup::destroy() {
    allocator_.destroy(*this);
}

packet::IWriter* ReceiverPortGroup::add_port(address::EndpointProtocol proto) {
    roc_log(LogInfo, "receiver: adding port %s", address::endpoint_proto_to_str(proto));

    core::SharedPtr<ReceiverPort> port = new (allocator_)
        ReceiverPort(proto, receiver_state_, session_group_, format_map_, allocator_);

    if (!port || !port->valid()) {
        return NULL;
    }

    ports_.push_back(*port);

    return port.get();
}

void ReceiverPortGroup::update(packet::timestamp_t timestamp) {
    core::SharedPtr<ReceiverPort> port;

    for (port = ports_.front(); port; port = ports_.nextof(*port)) {
        port->flush_packets();
    }

    session_group_.update_sessions(timestamp);
}

size_t ReceiverPortGroup::num_sessions() const {
    return session_group_.num_sessions();
}

} // namespace pipeline
} // namespace roc
