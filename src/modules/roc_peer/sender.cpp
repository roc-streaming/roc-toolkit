/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/sender.h"
#include "roc_address/endpoint_uri_to_str.h"
#include "roc_address/socket_addr.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

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

bool Sender::set_broadcast_enabled(address::Interface iface, bool enabled) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)ROC_ARRAY_SIZE(ports_));

    if (ports_[iface].handle) {
        roc_log(LogError,
                "sender peer:"
                " can't set broadcast flag for %s interface:"
                " interface is already bound",
                address::interface_to_str(iface));
        return false;
    }

    ports_[iface].config.broadcast_enabled = enabled;
    ports_[iface].is_set = true;

    roc_log(LogDebug, "sender peer: setting %s interface broadcast flag to %d",
            address::interface_to_str(iface), (int)enabled);

    return true;
}

bool Sender::set_outgoing_address(address::Interface iface, const char* ip) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)ROC_ARRAY_SIZE(ports_));

    roc_panic_if(!ip);

    if (ports_[iface].handle) {
        roc_log(LogError,
                "sender peer:"
                " can't set outgoing address for %s interface:"
                " interface is already bound",
                address::interface_to_str(iface));
        return false;
    }

    bool ok = ports_[iface].config.bind_address.set_host_port(
        address::Family_IPv4, ip, ports_[iface].config.bind_address.port());

    if (!ok) {
        ok = ports_[iface].config.bind_address.set_host_port(
            address::Family_IPv6, ip, ports_[iface].config.bind_address.port());
    }

    if (!ok) {
        roc_log(LogError,
                "sender peer:"
                " can't set outgoing address for %s interface to '%s':"
                " invalid IPv4 or IPv6 address",
                address::interface_to_str(iface), ip);
        return false;
    }

    ports_[iface].is_set = true;

    roc_log(LogDebug, "sender peer: setting %s interface outgoing address to %s",
            address::interface_to_str(iface), ip);

    return true;
}

bool Sender::connect(address::Interface iface, const address::EndpointURI& uri) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    if (!uri.check(address::EndpointURI::Subset_Full)) {
        roc_log(LogError, "sender peer: invalid uri");
        return false;
    }

    address::SocketAddr address;
    if (!context_.event_loop().resolve_endpoint_address(uri, address)) {
        roc_log(LogError, "sender peer: can't resolve %s interface address",
                address::interface_to_str(iface));
        return false;
    }

    address::Interface outgoing_iface = select_outgoing_iface_(iface);

    InterfacePort* port = setup_outgoing_iface_(outgoing_iface, address.family());
    if (!port) {
        roc_log(LogError, "sender peer: can't bind %s interface to local port",
                address::interface_to_str(iface));
        return false;
    }

    pipeline::SenderSink::EndpointHandle endpoint =
        pipeline_.add_endpoint(endpoint_set_, iface, uri.proto());

    if (!endpoint) {
        roc_log(LogError, "sender peer: can't add %s endpoint to pipeline",
                address::interface_to_str(iface));
        return false;
    }

    pipeline_.set_endpoint_destination_udp_address(endpoint, address);
    pipeline_.set_endpoint_output_writer(endpoint, *port->writer);

    if (iface == address::Iface_AudioSource) {
        source_endpoint_ = endpoint;
    } else {
        repair_endpoint_ = endpoint;
    }

    roc_log(LogInfo, "sender peer: connected %s interface to %s",
            address::interface_to_str(iface), address::endpoint_uri_to_str(uri).c_str());

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

address::Interface Sender::select_outgoing_iface_(address::Interface iface) {
    if (ports_[iface].is_set) {
        return iface;
    }

    return address::Iface_AudioCombined;
}

Sender::InterfacePort* Sender::setup_outgoing_iface_(address::Interface iface,
                                                     address::AddrFamily family) {
    InterfacePort& port = ports_[iface];

    if (port.config.bind_address.has_host_port()) {
        if (port.config.bind_address.family() != family) {
            roc_log(LogError,
                    "sender peer:"
                    " %s interface is configured to use %s,"
                    " but tried to be connected to %s address",
                    address::interface_to_str(iface),
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
            address::interface_to_str(iface),
            address::socket_addr_to_str(port.config.bind_address).c_str());

    return &port;
}

} // namespace peer
} // namespace roc
