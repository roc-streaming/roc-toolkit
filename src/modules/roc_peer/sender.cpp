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
    , repair_endpoint_(0) {
    roc_log(LogDebug, "sender peer: initializing");

    if (!pipeline_.valid()) {
        return;
    }

    endpoint_set_ = pipeline_.add_endpoint_set();
}

Sender::~Sender() {
    roc_log(LogDebug, "sender peer: deinitializing");

    for (size_t i = 0; i < ROC_ARRAY_SIZE(ports_); i++) {
        if (ports_[i].handle) {
            context_.event_loop().remove_port(ports_[i].handle);
        }
    }
}

bool Sender::valid() const {
    return endpoint_set_;
}

bool Sender::set_broadcast_enabled(address::EndpointType type, bool enabled) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    roc_panic_if(type < 0);
    roc_panic_if(type >= (int)ROC_ARRAY_SIZE(ports_));

    if (ports_[type].handle) {
        roc_log(LogError,
                "sender peer:"
                " can't set broadcast flag for %s interface:"
                " interface is already bound",
                address::endpoint_type_to_str(type));
        return false;
    }

    ports_[type].config.broadcast_enabled = enabled;
    ports_[type].is_set = true;

    roc_log(LogDebug, "sender peer: setting %s interface broadcast flag to %d",
            address::endpoint_type_to_str(type), (int)enabled);

    return true;
}

bool Sender::set_outgoing_address(address::EndpointType type, const char* ip) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    roc_panic_if(type < 0);
    roc_panic_if(type >= (int)ROC_ARRAY_SIZE(ports_));

    roc_panic_if(!ip);

    if (ports_[type].handle) {
        roc_log(LogError,
                "sender peer:"
                " can't set outgoing address for %s interface:"
                " interface is already bound",
                address::endpoint_type_to_str(type));
        return false;
    }

    bool ok = ports_[type].config.bind_address.set_host_port(
        address::Family_IPv4, ip, ports_[type].config.bind_address.port());

    if (!ok) {
        ok = ports_[type].config.bind_address.set_host_port(
            address::Family_IPv6, ip, ports_[type].config.bind_address.port());
    }

    if (!ok) {
        roc_log(LogError,
                "sender peer:"
                " can't set outgoing address for %s interface to '%s':"
                " invalid IPv4 or IPv6 address",
                address::endpoint_type_to_str(type), ip);
        return false;
    }

    ports_[type].is_set = true;

    roc_log(LogDebug, "sender peer: setting %s interface outgoing address to %s",
            address::endpoint_type_to_str(type), ip);

    return true;
}

bool Sender::connect(address::EndpointType type,
                     address::EndpointProtocol proto,
                     const address::SocketAddr& address) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    address::EndpointType outgoing_type = select_outgoing_port_(type);

    UdpPort* port = setup_outgoing_port_(outgoing_type, address.family());
    if (!port) {
        roc_log(LogError, "sender peer: can't bind %s interface to local port",
                address::endpoint_type_to_str(type));
        return false;
    }

    pipeline::SenderSink::EndpointHandle endpoint =
        pipeline_.add_endpoint(endpoint_set_, type, proto);

    if (!endpoint) {
        roc_log(LogError, "sender peer: can't add %s endpoint to pipeline",
                address::endpoint_type_to_str(type));
        return false;
    }

    pipeline_.set_endpoint_destination_udp_address(endpoint, address);
    pipeline_.set_endpoint_output_writer(endpoint, *port->writer);

    if (type == address::EndType_AudioSource) {
        source_endpoint_ = endpoint;
    } else {
        repair_endpoint_ = endpoint;
    }

    roc_log(LogInfo, "sender peer: connected %s interface to %s:%s",
            address::endpoint_type_to_str(type), address::endpoint_proto_to_str(proto),
            address::socket_addr_to_str(address).c_str());

    return true;
}

bool Sender::is_ready() const {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    return pipeline_.is_endpoint_set_ready(endpoint_set_);
}

sndio::ISink& Sender::sink() {
    roc_panic_if_not(valid());

    return pipeline_;
}

address::EndpointType Sender::select_outgoing_port_(address::EndpointType type) {
    if (ports_[type].is_set) {
        return type;
    }

    return address::EndType_AudioCombined;
}

Sender::UdpPort* Sender::setup_outgoing_port_(address::EndpointType type,
                                              address::AddrFamily family) {
    Sender::UdpPort& port = ports_[type];

    if (port.config.bind_address.has_host_port()) {
        if (port.config.bind_address.family() != family) {
            roc_log(LogError,
                    "sender peer:"
                    " %s interface is configured to use %s,"
                    " but tried to be connected to %s address",
                    address::endpoint_type_to_str(type),
                    address::addr_family_to_str(port.config.bind_address.family()),
                    address::addr_family_to_str(family));
            return NULL;
        }
    }

    if (port.handle) {
        return &port;
    }

    if (!port.config.bind_address.has_host_port()) {
        if (family == address::Family_IPv4) {
            port.config.bind_address.set_host_port(address::Family_IPv4, "0.0.0.0", 0);
        } else {
            port.config.bind_address.set_host_port(address::Family_IPv6, "::", 0);
        }
    }

    port.handle = context_.event_loop().add_udp_sender(port.config, &port.writer);
    if (!port.handle) {
        return NULL;
    }

    roc_log(LogInfo, "sender peer: bound %s interface to %s",
            address::endpoint_type_to_str(type),
            address::socket_addr_to_str(port.config.bind_address).c_str());

    return &port;
}

} // namespace peer
} // namespace roc
