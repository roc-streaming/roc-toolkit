/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_node/sender.h"
#include "roc_address/network_uri_to_str.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace node {

Sender::Sender(Context& context, const pipeline::SenderSinkConfig& pipeline_config)
    : Node(context)
    , pipeline_(*this,
                pipeline_config,
                context.processor_map(),
                context.encoding_map(),
                context.packet_pool(),
                context.packet_buffer_pool(),
                context.frame_pool(),
                context.frame_buffer_pool(),
                context.arena())
    , processing_task_(pipeline_)
    , slot_pool_("slot_pool", context.arena())
    , slot_map_(context.arena())
    , party_metrics_(context.arena())
    , frame_factory_(context.frame_pool(), context.frame_buffer_pool())
    , init_status_(status::NoStatus) {
    roc_log(LogDebug, "sender node: initializing");

    if ((init_status_ = pipeline_.init_status()) != status::StatusOK) {
        return;
    }

    sample_spec_ = pipeline_.sink().sample_spec();

    memset(used_interfaces_, 0, sizeof(used_interfaces_));
    memset(used_protocols_, 0, sizeof(used_protocols_));

    init_status_ = status::StatusOK;
}

Sender::~Sender() {
    roc_log(LogDebug, "sender node: deinitializing");

    // First remove all slots. This may involve usage of processing task.
    while (core::SharedPtr<Slot> slot = slot_map_.front()) {
        cleanup_slot_(*slot);
        slot_map_.remove(*slot);
    }

    // Then wait until processing task is fully completed, before
    // proceeding to its destruction.
    context().control_loop().wait(processing_task_);
}

status::StatusCode Sender::init_status() const {
    return init_status_;
}

bool Sender::configure(slot_index_t slot_index,
                       address::Interface iface,
                       const netio::UdpConfig& config) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogDebug, "sender node: configuring %s interface of slot %lu",
            address::interface_to_str(iface), (unsigned long)slot_index);

    core::SharedPtr<Slot> slot = get_slot_(slot_index, true);
    if (!slot) {
        roc_log(LogError,
                "sender node:"
                " can't configure %s interface of slot %lu:"
                " can't create slot",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (slot->broken) {
        roc_log(LogError,
                "sender node:"
                " can't configure %s interface of slot %lu:"
                " slot is marked broken and should be unlinked",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (slot->ports[iface].handle) {
        roc_log(LogError,
                "sender node:"
                " can't configure %s interface of slot %lu:"
                " interface is already bound or connected",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    slot->ports[iface].config = config;

    return true;
}

bool Sender::connect(slot_index_t slot_index,
                     address::Interface iface,
                     const address::NetworkUri& uri) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogInfo, "sender node: connecting %s interface of slot %lu to %s",
            address::interface_to_str(iface), (unsigned long)slot_index,
            address::network_uri_to_str(uri).c_str());

    core::SharedPtr<Slot> slot = get_slot_(slot_index, true);
    if (!slot) {
        roc_log(LogError,
                "sender node:"
                " can't connect %s interface of slot %lu:"
                " can't create slot",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (slot->broken) {
        roc_log(LogError,
                "sender node:"
                " can't connect %s interface of slot %lu:"
                " slot is marked broken and should be unlinked",
                address::interface_to_str(iface), (unsigned long)slot_index);
        return false;
    }

    if (!uri.verify(address::NetworkUri::Subset_Full)) {
        roc_log(LogError,
                "sender node: can't connect %s interface of slot %lu: invalid uri",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    if (!check_compatibility_(iface, uri)) {
        roc_log(LogError,
                "sender node:"
                " can't connect %s interface of slot %lu:"
                " incompatible with other slots",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    netio::NetworkLoop::Tasks::ResolveEndpointAddress resolve_task(uri);
    if (!context().network_loop().schedule_and_wait(resolve_task)) {
        roc_log(LogError,
                "sender node:"
                " can't connect %s interface of slot %lu:"
                " can't resolve endpoint address",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    const address::SocketAddr& address = resolve_task.get_address();

    Port& port = select_outgoing_port_(*slot, iface, address.family());

    if (!setup_outgoing_port_(port, iface, address.family())) {
        roc_log(LogError,
                "sender node:"
                " can't connect %s interface of slot %lu:"
                " can't setup local port",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    if (!port.handle) {
        netio::NetworkLoop::Tasks::AddUdpPort port_task(port.config);
        if (!context().network_loop().schedule_and_wait(port_task)) {
            roc_log(LogError,
                    "sender node:"
                    " can't connect %s interface of slot %lu:"
                    " can't bind to local port",
                    address::interface_to_str(iface), (unsigned long)slot_index);
            break_slot_(*slot);
            return false;
        }

        port.handle = port_task.get_handle();

        roc_log(LogInfo, "sender node: bound %s interface to %s",
                address::interface_to_str(iface),
                address::socket_addr_to_str(port.config.bind_address).c_str());
    }

    if (!port.outbound_writer) {
        netio::NetworkLoop::Tasks::StartUdpSend send_task(port.handle);
        if (!context().network_loop().schedule_and_wait(send_task)) {
            roc_log(LogError,
                    "sender node:"
                    " can't connect %s interface of slot %lu:"
                    " can't start sending on local port",
                    address::interface_to_str(iface), (unsigned long)slot_index);
            break_slot_(*slot);
            return false;
        }

        port.outbound_writer = &send_task.get_outbound_writer();
    }

    pipeline::SenderLoop::Tasks::AddEndpoint endpoint_task(
        slot->handle, iface, uri.proto(), address, *port.outbound_writer);
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError,
                "sender node:"
                " can't connect %s interface of slot %lu:"
                " can't add endpoint to pipeline",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    if (iface == address::Iface_AudioControl && endpoint_task.get_inbound_writer()) {
        netio::NetworkLoop::Tasks::StartUdpRecv recv_task(
            port.handle, *endpoint_task.get_inbound_writer());
        if (!context().network_loop().schedule_and_wait(recv_task)) {
            roc_log(LogError,
                    "sender node:"
                    " can't connect %s interface of slot %lu:"
                    " can't start receiving on local port",
                    address::interface_to_str(iface), (unsigned long)slot_index);
            break_slot_(*slot);
            return false;
        }
    }

    update_compatibility_(iface, uri);

    return true;
}

bool Sender::unlink(slot_index_t slot_index) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    roc_log(LogDebug, "sender node: unlinking slot %lu", (unsigned long)slot_index);

    core::SharedPtr<Slot> slot = get_slot_(slot_index, false);
    if (!slot) {
        roc_log(LogError,
                "sender node:"
                " can't unlink slot %lu: can't find slot",
                (unsigned long)slot_index);
        return false;
    }

    cleanup_slot_(*slot);
    slot_map_.remove(*slot);

    return true;
}

bool Sender::get_metrics(slot_index_t slot_index,
                         slot_metrics_func_t slot_metrics_func,
                         void* slot_metrics_arg,
                         party_metrics_func_t party_metrics_func,
                         size_t* party_metrics_size,
                         void* party_metrics_arg) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!slot_metrics_func);
    roc_panic_if(!party_metrics_func);

    core::SharedPtr<Slot> slot = get_slot_(slot_index, false);
    if (!slot) {
        roc_log(LogError,
                "sender node:"
                " can't get metrics of slot %lu: can't find slot",
                (unsigned long)slot_index);
        return false;
    }

    if (party_metrics_size) {
        if (!party_metrics_.resize(*party_metrics_size)) {
            roc_log(LogError,
                    "sender node:"
                    " can't get metrics of slot %lu: can't allocate buffer",
                    (unsigned long)slot_index);
            return false;
        }
    }

    pipeline::SenderLoop::Tasks::QuerySlot task(
        slot->handle, slot_metrics_,
        party_metrics_.size() != 0 ? party_metrics_.data() : NULL, party_metrics_size);
    if (!pipeline_.schedule_and_wait(task)) {
        roc_log(LogError,
                "sender node:"
                " can't get metrics of slot %lu: operation failed",
                (unsigned long)slot_index);
        return false;
    }

    if (slot_metrics_arg) {
        slot_metrics_func(slot_metrics_, slot_metrics_arg);
    }

    if (party_metrics_arg && party_metrics_size) {
        for (size_t party_index = 0; party_index < *party_metrics_size; party_index++) {
            party_metrics_func(party_metrics_[party_index], party_index,
                               party_metrics_arg);
        }
    }

    return true;
}

bool Sender::has_incomplete_slots() {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    for (core::SharedPtr<Slot> slot = slot_map_.front(); slot;
         slot = slot_map_.nextof(*slot)) {
        if (slot->broken) {
            return true;
        }

        if (slot->handle) {
            pipeline::SenderSlotMetrics slot_metrics;
            pipeline::SenderLoop::Tasks::QuerySlot task(slot->handle, slot_metrics, NULL,
                                                        NULL);
            if (!pipeline_.schedule_and_wait(task)) {
                return true;
            }
            if (!slot_metrics.is_complete) {
                return true;
            }
        }
    }

    return false;
}

bool Sender::has_broken_slots() {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    for (core::SharedPtr<Slot> slot = slot_map_.front(); slot;
         slot = slot_map_.nextof(*slot)) {
        if (slot->broken) {
            return true;
        }
    }

    return false;
}

status::StatusCode Sender::write_frame(const void* bytes, size_t n_bytes) {
    core::Mutex::Lock lock(frame_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!bytes);
    roc_panic_if(n_bytes == 0);

    if (!sample_spec_.is_valid_frame_size(n_bytes)) {
        return status::StatusBadBuffer;
    }

    if (!frame_) {
        if (!(frame_ = frame_factory_.allocate_frame_no_buffer())) {
            return status::StatusNoMem;
        }
    }

    core::BufferView frame_buffer(const_cast<void*>(bytes), n_bytes);

    frame_->set_buffer(frame_buffer);
    frame_->set_raw(sample_spec_.is_raw());
    frame_->set_duration(sample_spec_.bytes_2_stream_timestamp(n_bytes));

    const status::StatusCode code = pipeline_.sink().write(*frame_);

    // Detach buffer, clear frame for re-use.
    frame_->clear();

    return code;
}

sndio::ISink& Sender::sink() {
    roc_panic_if(init_status_ != status::StatusOK);

    return pipeline_.sink();
}

bool Sender::check_compatibility_(address::Interface iface,
                                  const address::NetworkUri& uri) {
    if (used_interfaces_[iface] && used_protocols_[iface] != uri.proto()) {
        roc_log(LogError,
                "sender node: same interface of all slots should use same protocols:"
                " other slot uses %s, but this slot tries to use %s",
                address::proto_to_str(used_protocols_[iface]),
                address::proto_to_str(uri.proto()));
        return false;
    }

    return true;
}

void Sender::update_compatibility_(address::Interface iface,
                                   const address::NetworkUri& uri) {
    used_interfaces_[iface] = true;
    used_protocols_[iface] = uri.proto();
}

core::SharedPtr<Sender::Slot> Sender::get_slot_(slot_index_t slot_index,
                                                bool auto_create) {
    core::SharedPtr<Slot> slot = slot_map_.find(slot_index);

    if (!slot) {
        if (auto_create) {
            pipeline::SenderSlotConfig slot_config;

            pipeline::SenderLoop::Tasks::CreateSlot slot_task(slot_config);
            if (!pipeline_.schedule_and_wait(slot_task)) {
                roc_log(LogError, "sender node: failed to create slot %lu",
                        (unsigned long)slot_index);
                return NULL;
            }

            slot = new (slot_pool_) Slot(slot_pool_, slot_index, slot_task.get_handle());
            if (!slot) {
                roc_log(LogError, "sender node: failed to create slot %lu",
                        (unsigned long)slot_index);
                return NULL;
            }

            if (!slot_map_.insert(*slot)) {
                roc_log(LogError, "sender node: failed to create slot %lu",
                        (unsigned long)slot_index);
                return NULL;
            }
        } else {
            roc_log(LogError, "sender node: failed to find slot %lu",
                    (unsigned long)slot_index);
            return NULL;
        }
    }

    return slot;
}

void Sender::cleanup_slot_(Slot& slot) {
    // First remove pipeline slot, because it writes to network ports.
    if (slot.handle) {
        pipeline::SenderLoop::Tasks::DeleteSlot task(slot.handle);
        if (!pipeline_.schedule_and_wait(task)) {
            roc_panic("sender node: can't remove pipeline slot %lu",
                      (unsigned long)slot.index);
        }
        slot.handle = NULL;
    }

    // Then remove network ports.
    for (size_t p = 0; p < address::Iface_Max; p++) {
        if (slot.ports[p].handle) {
            netio::NetworkLoop::Tasks::RemovePort task(slot.ports[p].handle);
            if (!context().network_loop().schedule_and_wait(task)) {
                roc_panic("sender node: can't remove network port of slot %lu",
                          (unsigned long)slot.index);
            }
            slot.ports[p].handle = NULL;
        }
    }
}

void Sender::break_slot_(Slot& slot) {
    roc_log(LogError, "sender node: marking slot %lu as broken, it needs to be unlinked",
            (unsigned long)slot.index);

    slot.broken = true;
    cleanup_slot_(slot);
}

Sender::Port& Sender::select_outgoing_port_(Slot& slot,
                                            address::Interface iface,
                                            address::AddrFamily family) {
    // We try to share outgoing port for source and repair interfaces, if they have
    // identical configuration. This should not harm, and it may help receiver to
    // associate source and repair streams together, in case when no control and
    // signaling protocol is used, by source addresses. This technique is neither
    // standard nor universal, but in many cases it allows us to work even without
    // protocols like RTCP or RTSP.
    const bool share_interface_ports =
        (iface == address::Iface_AudioSource || iface == address::Iface_AudioRepair
         || iface == address::Iface_AudioControl);

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

            roc_log(LogDebug, "sender node: sharing %s interface port with %s interface",
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
    if (port.config.bind_address) {
        if (port.config.bind_address.family() != family) {
            roc_log(LogError,
                    "sender node:"
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

        if (!port.config.bind_address) {
            if (family == address::Family_IPv4) {
                if (!port.config.bind_address.set_host_port(address::Family_IPv4,
                                                            "0.0.0.0", 0)) {
                    roc_panic("sender node: can't set reset %s interface ipv4 address",
                              address::interface_to_str(iface));
                }
            } else {
                if (!port.config.bind_address.set_host_port(address::Family_IPv6,
                                                            "::", 0)) {
                    roc_panic("sender node: can't set reset %s interface ipv6 address",
                              address::interface_to_str(iface));
                }
            }
        }
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

} // namespace node
} // namespace roc
