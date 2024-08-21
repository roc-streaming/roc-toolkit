/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_node/receiver.h"
#include "roc_address/network_uri_to_str.h"
#include "roc_address/socket_addr_to_str.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace node {

Receiver::Receiver(Context& context,
                   const pipeline::ReceiverSourceConfig& pipeline_config)
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
    roc_log(LogDebug, "receiver node: initializing");

    if ((init_status_ = pipeline_.init_status()) != status::StatusOK) {
        return;
    }

    sample_spec_ = pipeline_.source().sample_spec();

    init_status_ = status::StatusOK;
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

status::StatusCode Receiver::init_status() const {
    return init_status_;
}

bool Receiver::configure(slot_index_t slot_index,
                         address::Interface iface,
                         const netio::UdpConfig& config) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

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
                    address::NetworkUri& uri) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogInfo, "receiver node: binding %s interface of slot %lu to %s",
            address::interface_to_str(iface), (unsigned long)slot_index,
            address::network_uri_to_str(uri).c_str());

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

    if (!uri.verify(address::NetworkUri::Subset_Full)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " invalid uri",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    Port& port = slot->ports[iface];

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

    port.config.bind_address = resolve_task.get_address();

    netio::NetworkLoop::Tasks::AddUdpPort port_task(port.config);
    if (!context().network_loop().schedule_and_wait(port_task)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " can't bind interface to local port",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    port.handle = port_task.get_handle();

    packet::IWriter* outbound_writer = NULL;

    if (iface == address::Iface_AudioControl) {
        netio::NetworkLoop::Tasks::StartUdpSend send_task(port.handle);
        if (!context().network_loop().schedule_and_wait(send_task)) {
            roc_log(LogError,
                    "receiver node:"
                    " can't bind %s interface of slot %lu:"
                    " can't start sending on local port",
                    address::interface_to_str(iface), (unsigned long)slot_index);
            break_slot_(*slot);
            return false;
        }

        outbound_writer = &send_task.get_outbound_writer();
    }

    pipeline::ReceiverLoop::Tasks::AddEndpoint endpoint_task(
        slot->handle, iface, uri.proto(), port.config.bind_address, outbound_writer);
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " can't add endpoint to pipeline",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    netio::NetworkLoop::Tasks::StartUdpRecv recv_task(
        port.handle, *endpoint_task.get_inbound_writer());
    if (!context().network_loop().schedule_and_wait(recv_task)) {
        roc_log(LogError,
                "receiver node:"
                " can't bind %s interface of slot %lu:"
                " can't start receiving on local port",
                address::interface_to_str(iface), (unsigned long)slot_index);
        break_slot_(*slot);
        return false;
    }

    if (uri.port() == 0) {
        // Report back the port number we've selected.
        if (!uri.set_port(slot->ports[iface].config.bind_address.port())) {
            roc_panic("receiver node: can't set endpoint port");
        }
    }

    return true;
}

bool Receiver::unlink(slot_index_t slot_index) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

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
                "receiver node:"
                " can't get metrics of slot %lu: can't find slot",
                (unsigned long)slot_index);
        return false;
    }

    if (party_metrics_size) {
        if (!party_metrics_.resize(*party_metrics_size)) {
            roc_log(LogError,
                    "receiver node:"
                    " can't get metrics of slot %lu: can't allocate buffer",
                    (unsigned long)slot_index);
            return false;
        }
    }

    pipeline::ReceiverLoop::Tasks::QuerySlot task(
        slot->handle, slot_metrics_,
        party_metrics_.size() != 0 ? party_metrics_.data() : NULL, party_metrics_size);
    if (!pipeline_.schedule_and_wait(task)) {
        roc_log(LogError,
                "receiver node:"
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

bool Receiver::has_broken_slots() {
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

status::StatusCode Receiver::read_frame(void* bytes, size_t n_bytes) {
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

    // Attach pre-allocated buffer to frame.
    // This allows source to write result directly into user buffer.
    core::BufferView frame_buffer(bytes, n_bytes);
    frame_->set_buffer(frame_buffer);

    const status::StatusCode code = pipeline_.source().read(
        *frame_, sample_spec_.bytes_2_stream_timestamp(n_bytes), audio::ModeHard);

    if (code == status::StatusOK && frame_->bytes() != bytes) {
        // If source used another buffer, copy result from it.
        memmove(bytes, frame_->bytes(), n_bytes);
    }

    // Detach buffer, clear frame for re-use.
    frame_->clear();

    return code;
}

sndio::ISource& Receiver::source() {
    return pipeline_.source();
}

core::SharedPtr<Receiver::Slot> Receiver::get_slot_(slot_index_t slot_index,
                                                    bool auto_create) {
    core::SharedPtr<Slot> slot = slot_map_.find(slot_index);

    if (!slot) {
        if (auto_create) {
            pipeline::ReceiverSlotConfig slot_config;
            slot_config.enable_routing = true;

            pipeline::ReceiverLoop::Tasks::CreateSlot slot_task(slot_config);
            if (!pipeline_.schedule_and_wait(slot_task)) {
                roc_log(LogError, "receiver node: failed to create slot");
                return NULL;
            }

            slot = new (slot_pool_) Slot(slot_pool_, slot_index, slot_task.get_handle());
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
