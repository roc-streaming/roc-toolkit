/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_node/receiver.h"
#include "roc_address/endpoint_uri_to_str.h"
#include "roc_address/socket_addr.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"

namespace roc {
namespace node {

Receiver::Receiver(Context& context, const pipeline::ReceiverConfig& pipeline_config)
    : Node(context)
    , pipeline_(*this,
                pipeline_config,
                context.encoding_map(),
                context.packet_factory(),
                context.byte_buffer_factory(),
                context.sample_buffer_factory(),
                context.arena())
    , processing_task_(pipeline_)
    , slot_pool_("slot_pool", context.arena())
    , slot_map_(context.arena())
    , sess_metrics_(context.arena())
    , valid_(false) {
    roc_log(LogDebug, "receiver node: initializing");

    memset(used_interfaces_, 0, sizeof(used_interfaces_));
    memset(used_protocols_, 0, sizeof(used_protocols_));

    if (!pipeline_.is_valid()) {
        return;
    }

    valid_ = true;
}

Receiver::~Receiver() {
    roc_log(LogDebug, "receiver node: deinitializing");

    // First remove all slots. This may involve usage of processing task.
    while (core::SharedPtr<Slot> slot = slot_map_.front()) {
        cleanup_slot_(*slot);
        slot_map_.remove(*slot);
    }

    // Then wait until processing task is fully completed, before
    // proceeding to its destruction.
    context().control_loop().wait(processing_task_);
}

bool Receiver::is_valid() {
    return valid_;
}

bool Receiver::configure(slot_index_t slot_index,
                         address::Interface iface,
                         const netio::UdpReceiverConfig& config) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogDebug, "receiver node: configuring %s interface of slot %lu",
            address::interface_to_str(iface), (unsigned long)slot_index);

    core::SharedPtr<Slot> slot = get_slot_(slot_index, true);
    if (!slot) {
        roc_log(LogError,
                "receiver node:"
                " can't configure %s interface of slot %lu:"
                " can't create slot",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (slot->broken) {
        roc_log(LogError,
                "receiver node:"
                " can't configure %s interface of slot %lu:"
                " slot is marked broken and should be unlinked",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (slot->ports[iface].handle) {
        roc_log(LogError,
                "receiver node:"
                " can't configure %s interface of slot %lu:"
                " interface is already bound or connected",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    slot->ports[iface].config = config;

    return true;
}

bool Receiver::bind(slot_index_t slot_index,
                    address::Interface iface,
                    address::EndpointUri& uri) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogInfo, "receiver node: binding %s interface of slot %lu to %s",
            address::interface_to_str(iface), (unsigned long)slot_index,
            address::endpoint_uri_to_str(uri).c_str());

    core::SharedPtr<Slot> slot = get_slot_(slot_index, true);
    if (!slot) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " can't create slot",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (slot->broken) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " slot is marked broken and should be unlinked",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (!uri.verify(address::EndpointUri::Subset_Full)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " invalid uri",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    if (!check_compatibility_(iface, uri)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " incompatible with other slots",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    address::SocketAddr address;

    netio::NetworkLoop::Tasks::ResolveEndpointAddress resolve_task(uri);

    if (!context().network_loop().schedule_and_wait(resolve_task)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " can't resolve endpoint address",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    pipeline::ReceiverLoop::Tasks::AddEndpoint endpoint_task(slot->handle, iface,
                                                             uri.proto());
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " can't add endpoint to pipeline",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    slot->ports[iface].config.bind_address = resolve_task.get_address();

    netio::NetworkLoop::Tasks::AddUdpReceiverPort port_task(slot->ports[iface].config,
                                                            *endpoint_task.get_writer());
    if (!context().network_loop().schedule_and_wait(port_task)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " can't bind interface to local port",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    slot->ports[iface].handle = port_task.get_handle();

    if (uri.port() == 0) {
        // Report back the port number we've selected.
        if (!uri.set_port(slot->ports[iface].config.bind_address.port())) {
            roc_panic("receiver node: can't set endpoint port");
        }
    }

    update_compatibility_(iface, uri);

    return true;
}

bool Receiver::unlink(slot_index_t slot_index) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_log(LogDebug, "receiver node: unlinking slot %lu", (unsigned long)slot_index);

    core::SharedPtr<Slot> slot = get_slot_(slot_index, false);
    if (!slot) {
        roc_log(LogError,
                "receiver node:"
                " can't unlink slot %lu: can't find slot",
                (unsigned long)slot_index);
        return false;
    }

    cleanup_slot_(*slot);
    slot_map_.remove(*slot);

    return true;
}

bool Receiver::get_metrics(slot_index_t slot_index,
                           pipeline::ReceiverSlotMetrics& slot_metrics,
                           sess_metrics_func_t sess_metrics_func,
                           size_t* sess_metrics_size,
                           void* sess_metrics_arg) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(!sess_metrics_func);
    roc_panic_if(!sess_metrics_size);

    core::SharedPtr<Slot> slot = get_slot_(slot_index, false);
    if (!slot) {
        roc_log(LogError,
                "receiver node:"
                " can't get metrics of slot %lu: can't find slot",
                (unsigned long)slot_index);
        return false;
    }

    if (!sess_metrics_.resize(*sess_metrics_size)) {
        roc_log(LogError,
                "receiver node:"
                " can't get metrics of slot %lu: can't allocate buffer",
                (unsigned long)slot_index);
        return false;
    }

    pipeline::ReceiverLoop::Tasks::QuerySlot task(
        slot->handle, slot_metrics, sess_metrics_.data(), sess_metrics_size);
    if (!pipeline_.schedule_and_wait(task)) {
        roc_log(LogError,
                "receiver node:"
                " can't get metrics of slot %lu: operation failed",
                (unsigned long)slot_index);
        return false;
    }

    for (size_t sess_index = 0; sess_index < *sess_metrics_size; sess_index++) {
        sess_metrics_func(sess_metrics_[sess_index], sess_index, sess_metrics_arg);
    }

    return true;
}

bool Receiver::has_broken() {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    for (core::SharedPtr<Slot> slot = slot_map_.front(); slot;
         slot = slot_map_.nextof(*slot)) {
        if (slot->broken) {
            return true;
        }
    }

    return false;
}

sndio::ISource& Receiver::source() {
    return pipeline_.source();
}

bool Receiver::check_compatibility_(address::Interface iface,
                                    const address::EndpointUri& uri) {
    if (used_interfaces_[iface] && used_protocols_[iface] != uri.proto()) {
        roc_log(LogError,
                "receiver node: same interface of all slots should use same protocols:"
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

core::SharedPtr<Receiver::Slot> Receiver::get_slot_(slot_index_t slot_index,
                                                    bool auto_create) {
    core::SharedPtr<Slot> slot = slot_map_.find(slot_index);

    if (!slot) {
        if (auto_create) {
            pipeline::ReceiverLoop::Tasks::CreateSlot task;
            if (!pipeline_.schedule_and_wait(task)) {
                roc_log(LogError, "receiver node: failed to create slot");
                return NULL;
            }

            slot = new (slot_pool_) Slot(slot_pool_, slot_index, task.get_handle());
            if (!slot) {
                roc_log(LogError, "receiver node: failed to create slot %lu",
                        (unsigned long)slot_index);
                return NULL;
            }

            if (!slot_map_.insert(*slot)) {
                roc_log(LogError, "receiver node: failed to create slot %lu",
                        (unsigned long)slot_index);
                return NULL;
            }

        } else {
            roc_log(LogError, "receiver node: failed to find slot %lu",
                    (unsigned long)slot_index);
            return NULL;
        }
    }

    return slot;
}

void Receiver::cleanup_slot_(Slot& slot) {
    // First remove network ports, because they write to pipeline slot.
    for (size_t p = 0; p < address::Iface_Max; p++) {
        if (slot.ports[p].handle) {
            netio::NetworkLoop::Tasks::RemovePort task(slot.ports[p].handle);
            if (!context().network_loop().schedule_and_wait(task)) {
                roc_panic("receiver node: can't remove network port of slot %lu",
                          (unsigned long)slot.index);
            }
            slot.ports[p].handle = NULL;
        }
    }

    // Then remove pipeline slot.
    if (slot.handle) {
        pipeline::ReceiverLoop::Tasks::DeleteSlot task(slot.handle);
        if (!pipeline_.schedule_and_wait(task)) {
            roc_panic("receiver node: can't remove pipeline slot %lu",
                      (unsigned long)slot.index);
        }
        slot.handle = NULL;
    }
}

void Receiver::break_slot_(Slot& slot) {
    roc_log(LogError,
            "receiver node: marking slot %lu as broken, it needs to be unlinked",
            (unsigned long)slot.index);

    slot.broken = true;
    cleanup_slot_(slot);
}

void Receiver::schedule_task_processing(pipeline::PipelineLoop&,
                                        core::nanoseconds_t deadline) {
    context().control_loop().schedule_at(processing_task_, deadline, NULL);
}

void Receiver::cancel_task_processing(pipeline::PipelineLoop&) {
    context().control_loop().async_cancel(processing_task_);
}

} // namespace node
} // namespace roc
