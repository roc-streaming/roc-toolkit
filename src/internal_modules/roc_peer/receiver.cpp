/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/receiver.h"
#include "roc_address/endpoint_uri_to_str.h"
#include "roc_address/socket_addr.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"

namespace roc {
namespace peer {

Receiver::Receiver(Context& context, const pipeline::ReceiverConfig& pipeline_config)
    : BasicPeer(context)
    , pipeline_(*this,
                pipeline_config,
                context.format_map(),
                context.packet_factory(),
                context.byte_buffer_factory(),
                context.sample_buffer_factory(),
                context.allocator())
    , processing_task_(pipeline_)
    , valid_(false) {
    roc_log(LogDebug, "receiver peer: initializing");

    memset(used_interfaces_, 0, sizeof(used_interfaces_));
    memset(used_protocols_, 0, sizeof(used_protocols_));

    if (!pipeline_.is_valid()) {
        return;
    }

    valid_ = true;
}

Receiver::~Receiver() {
    roc_log(LogDebug, "receiver peer: deinitializing");

    context().control_loop().wait(processing_task_);

    for (size_t s = 0; s < slots_.size(); s++) {
        if (!slots_[s].slot) {
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

bool Receiver::is_valid() {
    return valid_;
}

bool Receiver::configure(size_t slot_index,
                         address::Interface iface,
                         const netio::UdpReceiverConfig& config) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogDebug, "receiver peer: configuring %s interface of slot %lu",
            address::interface_to_str(iface), (unsigned long)slot_index);

    Slot* slot = get_slot_(slot_index, true);
    if (!slot) {
        roc_log(LogError,
                "receiver peer:"
                " can't configure %s interface of slot %lu:"
                " can't create slot",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (slot->ports[iface].handle) {
        roc_log(LogError,
                "receiver peer:"
                " can't configure %s interface of slot %lu:"
                " interface is already bound or connected",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    slot->ports[iface].config = config;

    return true;
}

bool Receiver::bind(size_t slot_index,
                    address::Interface iface,
                    address::EndpointUri& uri) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogInfo, "receiver peer: binding %s interface of slot %lu to %s",
            address::interface_to_str(iface), (unsigned long)slot_index,
            address::endpoint_uri_to_str(uri).c_str());

    if (!uri.verify(address::EndpointUri::Subset_Full)) {
        roc_log(LogError,
                "receiver peer:"
                " can't bind %s interface of slot %lu:"
                " invalid uri",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (!check_compatibility_(iface, uri)) {
        roc_log(LogError,
                "receiver peer:"
                " can't bind %s interface of slot %lu:"
                " incompatible with other slots",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    Slot* slot = get_slot_(slot_index, true);
    if (!slot) {
        roc_log(LogError,
                "receiver peer:"
                " can't bind %s interface of slot %lu:"
                " can't create slot",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    address::SocketAddr address;

    netio::NetworkLoop::Tasks::ResolveEndpointAddress resolve_task(uri);

    if (!context().network_loop().schedule_and_wait(resolve_task)) {
        roc_log(LogError,
                "receiver peer:"
                " can't bind %s interface of slot %lu:"
                " can't resolve endpoint address",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    pipeline::ReceiverLoop::Tasks::CreateEndpoint endpoint_task(slot->slot, iface,
                                                                uri.proto());
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError,
                "receiver peer:"
                " can't bind %s interface of slot %lu:"
                " can't add endpoint to pipeline",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    slot->ports[iface].config.bind_address = resolve_task.get_address();

    netio::NetworkLoop::Tasks::AddUdpReceiverPort port_task(slot->ports[iface].config,
                                                            *endpoint_task.get_writer());
    if (!context().network_loop().schedule_and_wait(port_task)) {
        roc_log(LogError,
                "receiver peer:"
                " can't bind %s interface of slot %lu:"
                " can't bind interface to local port",
                address::interface_to_str(iface), (unsigned long)slot_index);

        pipeline::ReceiverLoop::Tasks::DeleteEndpoint delete_endpoint_task(slot->slot,
                                                                           iface);
        if (!pipeline_.schedule_and_wait(delete_endpoint_task)) {
            roc_panic("receiver peer: can't remove newly created endpoint");
        }

        return false;
    }

    slot->ports[iface].handle = port_task.get_handle();

    if (uri.port() == 0) {
        // Report back the port number we've selected.
        if (!uri.set_port(slot->ports[iface].config.bind_address.port())) {
            roc_panic("receiver peer: can't set endpoint port");
        }
    }

    update_compatibility_(iface, uri);

    return true;
}

sndio::ISource& Receiver::source() {
    return pipeline_.source();
}

bool Receiver::check_compatibility_(address::Interface iface,
                                    const address::EndpointUri& uri) {
    if (used_interfaces_[iface] && used_protocols_[iface] != uri.proto()) {
        roc_log(LogError,
                "receiver peer: same interface of all slots should use same protocols:"
                " other slot uses %s, but this slot tries to use %s",
                address::proto_to_str(used_protocols_[iface]),
                address::proto_to_str(uri.proto()));
        return false;
    }

    return true;
}

void Receiver::update_compatibility_(address::Interface iface,
                                     const address::EndpointUri& uri) {
    used_interfaces_[iface] = true;
    used_protocols_[iface] = uri.proto();
}

Receiver::Slot* Receiver::get_slot_(size_t slot_index, bool auto_create) {
    if (slots_.size() <= slot_index) {
        if (!auto_create) {
            roc_log(LogError, "receiver peer: failed to find slot %lu",
                    (unsigned long)slot_index);
            return NULL;
        }
        if (!slots_.resize(slot_index + 1)) {
            roc_log(LogError, "receiver peer: failed to allocate slot");
            return NULL;
        }
    }

    if (!slots_[slot_index].slot) {
        if (!auto_create) {
            roc_log(LogError, "receiver peer: failed to find slot %lu",
                    (unsigned long)slot_index);
            return NULL;
        }
        pipeline::ReceiverLoop::Tasks::CreateSlot task;
        if (!pipeline_.schedule_and_wait(task)) {
            roc_log(LogError, "receiver peer: failed to create slot");
            return NULL;
        }
        slots_[slot_index].slot = task.get_handle();
    }

    return &slots_[slot_index];
}

void Receiver::schedule_task_processing(pipeline::PipelineLoop&,
                                        core::nanoseconds_t deadline) {
    context().control_loop().schedule_at(processing_task_, deadline, NULL);
}

void Receiver::cancel_task_processing(pipeline::PipelineLoop&) {
    context().control_loop().async_cancel(processing_task_);
}

} // namespace peer
} // namespace roc
