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

namespace roc {
namespace peer {

Sender::Sender(Context& context, const pipeline::SenderConfig& pipeline_config)
    : BasicPeer(context)
    , pipeline_(pipeline_config,
                format_map_,
                context_.packet_pool(),
                context_.byte_buffer_pool(),
                context_.sample_buffer_pool(),
                context_.allocator())
    , endpoint_set_(0)
    , source_endpoint_(0)
    , repair_endpoint_(0)
    , udp_writer_(NULL) {
    roc_log(LogDebug, "sender peer: initializing");

    if (!pipeline_.valid()) {
        return;
    }

    endpoint_set_ = pipeline_.add_endpoint_set();
}

Sender::~Sender() {
    roc_log(LogDebug, "sender peer: deinitializing");

    if (udp_writer_) {
        context_.event_loop().remove_port(bind_address_);
    }
}

bool Sender::valid() const {
    return endpoint_set_;
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

    if (source_endpoint_) {
        pipeline_.set_endpoint_output_writer(source_endpoint_, *udp_writer_);
    }

    if (repair_endpoint_) {
        pipeline_.set_endpoint_output_writer(repair_endpoint_, *udp_writer_);
    }

    roc_log(LogInfo, "sender peer: bound to %s",
            address::socket_addr_to_str(bind_address_).c_str());

    return true;
}

bool Sender::connect(address::EndpointType type,
                     address::EndpointProtocol proto,
                     const address::SocketAddr& address) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    pipeline::SenderSink::EndpointHandle endpoint =
        pipeline_.add_endpoint(endpoint_set_, type, proto);

    if (!endpoint) {
        roc_log(LogError, "sender peer: can't add endpoint to pipeline");
        return false;
    }

    pipeline_.set_endpoint_destination_udp_address(endpoint, address);

    if (udp_writer_) {
        pipeline_.set_endpoint_output_writer(endpoint, *udp_writer_);
    }

    if (type == address::EndType_AudioSource) {
        source_endpoint_ = endpoint;
    } else {
        repair_endpoint_ = endpoint;
    }

    roc_log(LogInfo, "sender peer: connected to %s endpoint %s at %s",
            address::endpoint_type_to_str(type), address::endpoint_proto_to_str(proto),
            address::socket_addr_to_str(address).c_str());

    return true;
}

sndio::ISink& Sender::sink() {
    roc_panic_if_not(valid());

    return pipeline_;
}

bool Sender::is_ready() const {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    return pipeline_.endpoint_set_ready(endpoint_set_);
}

} // namespace peer
} // namespace roc
