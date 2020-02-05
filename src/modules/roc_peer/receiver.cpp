/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/receiver.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/helpers.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

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
    , endpoint_set_(0) {
    roc_log(LogDebug, "receiver peer: initializing");

    if (!pipeline_.valid()) {
        return;
    }

    endpoint_set_ = pipeline_.add_endpoint_set();
}

Receiver::~Receiver() {
    roc_log(LogDebug, "receiver peer: deinitializing");

    for (size_t i = 0; i < ROC_ARRAY_SIZE(ports_); i++) {
        if (ports_[i].handle) {
            context_.event_loop().remove_port(ports_[i].handle);
        }
    }
}

bool Receiver::valid() {
    return endpoint_set_;
}

bool Receiver::set_multicast_group(address::Interface iface, const char* ip) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)ROC_ARRAY_SIZE(ports_));

    if (ports_[iface].handle) {
        roc_log(LogError,
                "receiver peer:"
                " can't set multicast group for %s interface:"
                " interface is already bound",
                address::interface_to_str(iface));
        return false;
    }

    core::StringBuilder b(ports_[iface].config.multicast_interface,
                          sizeof(ports_[iface].config.multicast_interface));

    if (!b.set_str(ip)) {
        roc_log(LogError,
                "receiver peer:"
                " can't set multicast group for %s interface to '%s':"
                " invalid IPv4 or IPv6 address",
                address::interface_to_str(iface), ip);
        return false;
    }

    roc_log(LogDebug, "receiver peer: setting %s interface multicast group to %s",
            address::interface_to_str(iface), ip);

    return true;
}

bool Receiver::bind(address::Interface iface,
                    address::Protocol proto,
                    address::SocketAddr& address) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(valid());

    packet::IWriter* endpoint_writer =
        pipeline_.add_endpoint(endpoint_set_, iface, proto);
    if (!endpoint_writer) {
        roc_log(LogError, "receiver peer: can't add %s endpoint to pipeline",
                address::interface_to_str(iface));
        return false;
    }

    ports_[iface].config.bind_address = address;

    ports_[iface].handle =
        context_.event_loop().add_udp_receiver(ports_[iface].config, *endpoint_writer);

    if (!ports_[iface].handle) {
        roc_log(LogError, "receiver peer: can't bind %s interface to local port",
                address::interface_to_str(iface));
        return false;
    }

    address = ports_[iface].config.bind_address;

    roc_log(LogInfo, "receiver peer: bound %s interface to %s:%s",
            address::interface_to_str(iface), address::proto_to_str(proto),
            address::socket_addr_to_str(address).c_str());

    return true;
}

sndio::ISource& Receiver::source() {
    return pipeline_;
}

} // namespace peer
} // namespace roc
