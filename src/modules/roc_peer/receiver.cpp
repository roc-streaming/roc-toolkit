/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/receiver.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_pipeline/port_to_str.h"

namespace roc {
namespace peer {

Receiver::Receiver(Context& context, const pipeline::ReceiverConfig& pipeline_config)
    : BasicPeer(context)
    , pipeline_(pipeline_config,
                format_map_,
                context_.packet_pool(),
                context_.byte_buffer_pool(),
                context_.sample_buffer_pool(),
                context_.allocator())
    , default_port_group_(0)
    , addresses_(context_.allocator()) {
    roc_log(LogDebug, "receiver peer: initializing");

    if (!pipeline_.valid()) {
        return;
    }

    default_port_group_ = pipeline_.add_port_group();
}

Receiver::~Receiver() {
    roc_log(LogDebug, "receiver peer: deinitializing");

    for (size_t i = 0; i < addresses_.size(); i++) {
        context_.event_loop().remove_port(addresses_[i]);
    }
}

bool Receiver::valid() {
    return default_port_group_;
}

bool Receiver::bind(address::EndpointType, pipeline::PortConfig& port_config) {
    core::Mutex::Lock lock(mutex_);

    if (!addresses_.grow_exp((addresses_.size() + 1))) {
        roc_log(LogError, "receiver peer: can't grow the addresses array");
        return false;
    }

    packet::IWriter* port_writer =
        pipeline_.add_port(default_port_group_, port_config.protocol);

    if (!port_writer) {
        roc_log(LogError, "receiver peer: can't add port to pipeline");
        return false;
    }

    if (!context_.event_loop().add_udp_receiver(port_config.address, *port_writer)) {
        roc_log(LogError, "receiver peer: bind failed");
        return false;
    }

    addresses_.push_back(port_config.address);

    roc_log(LogInfo, "receiver peer: bound to %s",
            pipeline::port_to_str(port_config).c_str());

    return true;
}

sndio::ISource& Receiver::source() {
    return pipeline_;
}

} // namespace peer
} // namespace roc
