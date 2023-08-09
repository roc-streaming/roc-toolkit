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
                context.arena())
    , processing_task_(pipeline_)
    , slot_pool_(context.arena())
    , slot_map_(context.arena())
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

    while (!slot_map_.is_empty()) {
        remove_slot_(slot_map_.back());
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

    core::SharedPtr<Slot> slot = get_slot_(slot_index, true);
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

    core::SharedPtr<Slot> slot = get_slot_(slot_index, true);
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

    pipeline::ReceiverLoop::Tasks::CreateEndpoint endpoint_task(slot->handle, iface,
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

        pipeline::ReceiverLoop::Tasks::DeleteEndpoint delete_endpoint_task(slot->handle,
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

bool Receiver::unlink(size_t slot_index) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_log(LogDebug, "receiver peer: unlinking slot %lu", (unsigned long)slot_index);

    core::SharedPtr<Slot> slot = get_slot_(slot_index, false);
    if (!slot) {
        roc_log(LogError,
                "receiver peer:"
                " can't unlink slot %lu: can't find slot",
                (unsigned long)slot_index);
        return false;
    }

    remove_slot_(slot);
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

core::SharedPtr<Receiver::Slot> Receiver::get_slot_(size_t slot_index, bool auto_create) {
    core::SharedPtr<Slot> slot = slot_map_.find(slot_index);

    if (!slot) {
        if (auto_create) {
            pipeline::ReceiverLoop::Tasks::CreateSlot task;
            if (!pipeline_.schedule_and_wait(task)) {
                roc_log(LogError, "receiver peer: failed to create slot");
                return NULL;
            }

            slot = new (slot_pool_) Slot(slot_pool_, slot_index, task.get_handle());
            if (!slot) {
                roc_log(LogError, "receiver peer: failed to create slot %lu",
                        (unsigned long)slot_index);
                return NULL;
            }

            if (!slot_map_.grow()) {
                roc_log(LogError, "receiver peer: failed to create slot %lu",
                        (unsigned long)slot_index);
                return NULL;
            }

            slot_map_.insert(*slot);
        } else {
            roc_log(LogError, "receiver peer: failed to find slot %lu",
                    (unsigned long)slot_index);
            return NULL;
        }
    }

    return slot;
}

void Receiver::remove_slot_(const core::SharedPtr<Slot>& slot) {
    // First remove network ports, because they write to pipeline slot.
    for (size_t p = 0; p < address::Iface_Max; p++) {
        if (slot->ports[p].handle) {
            netio::NetworkLoop::Tasks::RemovePort task(slot->ports[p].handle);
            if (!context().network_loop().schedule_and_wait(task)) {
                roc_panic("receiver peer: can't remove network port of slot %lu",
                          (unsigned long)slot->index);
            }
        }
    }

    // Then remove pipeline slot.
    if (slot->handle) {
        pipeline::ReceiverLoop::Tasks::DeleteSlot task(slot->handle);
        if (!pipeline_.schedule_and_wait(task)) {
            roc_panic("receiver peer: can't remove pipeline slot %lu",
                      (unsigned long)slot->index);
        }
    }

    slot_map_.remove(*slot);
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
