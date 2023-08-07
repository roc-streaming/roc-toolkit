/*
 * Copyright (c) 2020 Roc Streaming authors
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
    , pipeline_(*this,
                pipeline_config,
                context.format_map(),
                context.packet_factory(),
                context.byte_buffer_factory(),
                context.sample_buffer_factory(),
                context.allocator())
    , processing_task_(pipeline_)
    , slots_(context.allocator())
    , valid_(false) {
    roc_log(LogDebug, "sender peer: initializing");

    memset(used_interfaces_, 0, sizeof(used_interfaces_));
    memset(used_protocols_, 0, sizeof(used_protocols_));

    if (!pipeline_.is_valid()) {
        return;
    }

    valid_ = true;
}

Sender::~Sender() {
    roc_log(LogDebug, "sender peer: deinitializing");

    context().control_loop().wait(processing_task_);

    for (size_t s = 0; s < slots_.size(); s++) {
        if (!slots_[s].handle) {
            continue;
        }

        for (size_t p = 0; p < address::Iface_Max; p++) {
            if (!slots_[s].ports[p].handle) {
                continue;
            }

            netio::NetworkLoop::Tasks::RemovePort task(slots_[s].ports[p].handle);
            if (!context().network_loop().schedule_and_wait(task)) {
                roc_panic("sender peer: can't remove port");
            }
        }
    }
}

bool Sender::is_valid() const {
    return valid_;
}

bool Sender::configure(size_t slot_index,
                       address::Interface iface,
                       const netio::UdpSenderConfig& config) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogDebug, "sender peer: configuring %s interface of slot %lu",
            address::interface_to_str(iface), (unsigned long)slot_index);

    Slot* slot = get_slot_(slot_index, true);
    if (!slot) {
        roc_log(LogError,
                "sender peer:"
                " can't configure %s interface of slot %lu:"
                " can't create slot",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (slot->ports[iface].handle) {
        roc_log(LogError,
                "sender peer:"
                " can't configure %s interface of slot %lu:"
                " interface is already bound or connected",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    slot->ports[iface].config = config;

    return true;
}

bool Sender::connect(size_t slot_index,
                     address::Interface iface,
                     const address::EndpointUri& uri) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogInfo, "sender peer: connecting %s interface of slot %lu to %s",
            address::interface_to_str(iface), (unsigned long)slot_index,
            address::endpoint_uri_to_str(uri).c_str());

    if (!uri.verify(address::EndpointUri::Subset_Full)) {
        roc_log(LogError,
                "sender peer: can't connect %s interface of slot %lu: invalid uri",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (!check_compatibility_(iface, uri)) {
        roc_log(LogError,
                "sender peer:"
                " can't connect %s interface of slot %lu:"
                " incompatible with other slots",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    Slot* slot = get_slot_(slot_index, true);
    if (!slot) {
        roc_log(LogError,
                "sender peer:"
                " can't connect %s interface of slot %lu:"
                " can't create slot",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    netio::NetworkLoop::Tasks::ResolveEndpointAddress resolve_task(uri);

    if (!context().network_loop().schedule_and_wait(resolve_task)) {
        roc_log(LogError,
                "sender peer:"
                " can't connect %s interface of slot %lu:"
                " can't resolve endpoint address",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    const address::SocketAddr& address = resolve_task.get_address();

    Port& port = select_outgoing_port_(*slot, iface, address.family());

    if (!setup_outgoing_port_(port, iface, address.family())) {
        roc_log(LogError,
                "sender peer:"
                " can't connect %s interface of slot %lu:"
                " can't bind to local port",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    pipeline::SenderLoop::Tasks::CreateEndpoint endpoint_task(slot->handle, iface,
                                                              uri.proto());
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError,
                "sender peer:"
                " can't connect %s interface of slot %lu:"
                " can't add endpoint to pipeline",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    pipeline::SenderLoop::Tasks::SetEndpointDestinationAddress address_task(
        endpoint_task.get_handle(), address);

    if (!pipeline_.schedule_and_wait(address_task)) {
        roc_log(LogError,
                "sender peer:"
                " can't connect %s interface of slot %lu:"
                " can't set endpoint destination address",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    pipeline::SenderLoop::Tasks::SetEndpointDestinationWriter writer_task(
        endpoint_task.get_handle(), *port.writer);

    if (!pipeline_.schedule_and_wait(writer_task)) {
        roc_log(LogError,
                "sender peer:"
                " can't connect %s interface of slot %lu:"
                " can't set endpoint destination writer",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    update_compatibility_(iface, uri);

    return true;
}

bool Sender::is_ready() {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    if (slots_.size() == 0) {
        return false;
    }

    for (size_t s = 0; s < slots_.size(); s++) {
        if (!slots_[s].handle) {
            continue;
        }

        pipeline::SenderLoop::Tasks::CheckSlotIsReady task(slots_[s].handle);
        if (!pipeline_.schedule_and_wait(task)) {
            return false;
        }
    }

    return true;
}

sndio::ISink& Sender::sink() {
    roc_panic_if_not(is_valid());

    return pipeline_.sink();
}

bool Sender::check_compatibility_(address::Interface iface,
                                  const address::EndpointUri& uri) {
    if (used_interfaces_[iface] && used_protocols_[iface] != uri.proto()) {
        roc_log(LogError,
                "sender peer: same interface of all slots should use same protocols:"
                " other slot uses %s, but this slot tries to use %s",
                address::proto_to_str(used_protocols_[iface]),
                address::proto_to_str(uri.proto()));
        return false;
    }

    return true;
}

void Sender::update_compatibility_(address::Interface iface,
                                   const address::EndpointUri& uri) {
    used_interfaces_[iface] = true;
    used_protocols_[iface] = uri.proto();
}

Sender::Slot* Sender::get_slot_(size_t slot_index, bool auto_create) {
    if (slots_.size() <= slot_index) {
        if (!auto_create) {
            roc_log(LogError, "sender peer: failed to find slot %lu",
                    (unsigned long)slot_index);
            return NULL;
        }
        if (!slots_.resize(slot_index + 1)) {
            roc_log(LogError, "sender peer: failed to allocate slot");
            return NULL;
        }
    }

    if (!slots_[slot_index].handle) {
        if (!auto_create) {
            roc_log(LogError, "sender peer: failed to find slot %lu",
                    (unsigned long)slot_index);
            return NULL;
        }
        pipeline::SenderLoop::Tasks::CreateSlot task;
        if (!pipeline_.schedule_and_wait(task)) {
            roc_log(LogError, "sender peer: failed to create slot");
            return NULL;
        }
        slots_[slot_index].handle = task.get_handle();
    }

    return &slots_[slot_index];
}

Sender::Port& Sender::select_outgoing_port_(Slot& slot,
                                            address::Interface iface,
                                            address::AddrFamily family) {
    // We try to share outgoing port for source and repair interfaces, if they have
    // identical configuratrion. This should not harm, and it may help receiver to
    // associate source and repair streams together, in case when no control and
    // signaling protocol is used, by source addresses. This technique is neither
    // standard nor universal, but in many cases it allows us to work even without
    // protocols like RTCP or RTSP.
    const bool share_interface_ports =
        (iface == address::Iface_AudioSource || iface == address::Iface_AudioRepair);

    if (share_interface_ports && !slot.ports[iface].handle) {
        for (size_t i = 0; i < address::Iface_Max; i++) {
            if ((int)i == iface) {
                continue;
            }
            if (!slot.ports[i].handle) {
                continue;
            }
            if (!(slot.ports[i].orig_config == slot.ports[iface].config)) {
                continue;
            }
            if (!(slot.ports[i].config.bind_address.family() == family)) {
                continue;
            }

            roc_log(LogDebug, "sender peer: sharing %s interface port with %s interface",
                    address::interface_to_str(address::Interface(i)),
                    address::interface_to_str(iface));

            return slot.ports[i];
        }
    }

    return slot.ports[iface];
}

bool Sender::setup_outgoing_port_(Port& port,
                                  address::Interface iface,
                                  address::AddrFamily family) {
    if (port.config.bind_address.has_host_port()) {
        if (port.config.bind_address.family() != family) {
            roc_log(LogError,
                    "sender peer:"
                    " %s interface is configured to use %s,"
                    " but tried to be connected to %s address",
                    address::interface_to_str(iface),
                    address::addr_family_to_str(port.config.bind_address.family()),
                    address::addr_family_to_str(family));
            return false;
        }
    }

    if (!port.handle) {
        port.orig_config = port.config;

        if (!port.config.bind_address.has_host_port()) {
            if (family == address::Family_IPv4) {
                if (!port.config.bind_address.set_host_port(address::Family_IPv4,
                                                            "0.0.0.0", 0)) {
                    roc_panic("sender peer: can't set reset %s interface ipv4 address",
                              address::interface_to_str(iface));
                }
            } else {
                if (!port.config.bind_address.set_host_port(address::Family_IPv6,
                                                            "::", 0)) {
                    roc_panic("sender peer: can't set reset %s interface ipv6 address",
                              address::interface_to_str(iface));
                }
            }
        }

        netio::NetworkLoop::Tasks::AddUdpSenderPort port_task(port.config);

        if (!context().network_loop().schedule_and_wait(port_task)) {
            roc_log(LogError, "sender peer: can't bind %s interface to local port",
                    address::interface_to_str(iface));
            return false;
        }

        port.handle = port_task.get_handle();
        port.writer = port_task.get_writer();

        roc_log(LogInfo, "sender peer: bound %s interface to %s",
                address::interface_to_str(iface),
                address::socket_addr_to_str(port.config.bind_address).c_str());
    }

    return true;
}

void Sender::schedule_task_processing(pipeline::PipelineLoop&,
                                      core::nanoseconds_t deadline) {
    context().control_loop().schedule_at(processing_task_, deadline, NULL);
}

void Sender::cancel_task_processing(pipeline::PipelineLoop&) {
    context().control_loop().async_cancel(processing_task_);
}

} // namespace peer
} // namespace roc
