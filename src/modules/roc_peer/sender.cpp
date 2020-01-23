/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/sender.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_pipeline/port_to_str.h"

namespace roc {
namespace peer {

Sender::Sender(Context& context, const pipeline::SenderConfig& pipeline_config)
    : BasicPeer(context)
    , pipeline_(pipeline_config,
                codec_map_,
                format_map_,
                context_.packet_pool(),
                context_.byte_buffer_pool(),
                context_.sample_buffer_pool(),
                context_.allocator())
    , default_port_group_(0)
    , source_id_(0)
    , repair_id_(0)
    , udp_writer_(NULL) {
    roc_log(LogDebug, "sender peer: initializing");

    if (!pipeline_.valid()) {
        return;
    }

    default_port_group_ = pipeline_.add_port_group();
}

Sender::~Sender() {
    roc_log(LogDebug, "sender peer: deinitializing");

    if (udp_writer_) {
        context_.event_loop().remove_port(bind_address_);
    }
}

bool Sender::valid() const {
    return default_port_group_;
}

bool Sender::bind(address::SocketAddr& addr) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    if (udp_writer_) {
        roc_log(LogError, "sender peer: already bound");
        return false;
    }

    udp_writer_ = context_.event_loop().add_udp_sender(addr);
    if (!udp_writer_) {
        roc_log(LogError, "sender peer: bind failed");
        return false;
    }

    bind_address_ = addr;

    if (source_id_) {
        pipeline_.set_port_writer(source_id_, *udp_writer_);
    }

    if (repair_id_) {
        pipeline_.set_port_writer(repair_id_, *udp_writer_);
    }

    roc_log(LogInfo, "sender peer: bound to %s",
            address::socket_addr_to_str(bind_address_).c_str());

    return true;
}

bool Sender::connect(address::EndpointType port_type,
                     const pipeline::PortConfig& port_config) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    pipeline::SenderSink::PortID port_id =
        pipeline_.add_port(default_port_group_, port_type, port_config);

    if (!port_id) {
        roc_log(LogError, "sender peer: connect failed");
        return false;
    }

    if (udp_writer_) {
        pipeline_.set_port_writer(port_id, *udp_writer_);
    }

    if (port_type == address::EndType_AudioSource) {
        source_id_ = port_id;
    } else {
        repair_id_ = port_id;
    }

    roc_log(LogInfo, "sender peer: connected to %s",
            pipeline::port_to_str(port_config).c_str());

    return true;
}

sndio::ISink& Sender::sink() {
    roc_panic_if_not(valid());

    return pipeline_;
}

bool Sender::is_configured() const {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    return pipeline_.port_group_configured(default_port_group_);
}

} // namespace peer
} // namespace roc
