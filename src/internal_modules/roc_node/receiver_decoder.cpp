/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_node/receiver_decoder.h"
#include "roc_address/interface.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace node {

ReceiverDecoder::ReceiverDecoder(Context& context,
                                 const pipeline::ReceiverSourceConfig& pipeline_config)
    : Node(context)
    , pipeline_(*this,
                pipeline_config,
                context.encoding_map(),
                context.packet_factory(),
                context.byte_buffer_factory(),
                context.sample_buffer_factory(),
                context.arena())
    , slot_(NULL)
    , processing_task_(pipeline_)
    , party_metrics_(context.arena())
    , valid_(false) {
    roc_log(LogDebug, "receiver decoder node: initializing");

    if (!pipeline_.is_valid()) {
        roc_log(LogError, "receiver decoder node: failed to construct pipeline");
        return;
    }

    pipeline::ReceiverSlotConfig slot_config;
    slot_config.enable_routing = false;

    pipeline::ReceiverLoop::Tasks::CreateSlot slot_task(slot_config);
    if (!pipeline_.schedule_and_wait(slot_task)) {
        roc_log(LogError, "receiver decoder node: failed to create slot");
        return;
    }

    slot_ = slot_task.get_handle();
    if (!slot_) {
        roc_log(LogError, "receiver decoder node: failed to create slot");
        return;
    }

    valid_ = true;
}

ReceiverDecoder::~ReceiverDecoder() {
    roc_log(LogDebug, "receiver decoder node: deinitializing");

    if (slot_) {
        // First remove slot. This may involve usage of processing task.
        pipeline::ReceiverLoop::Tasks::DeleteSlot task(slot_);
        if (!pipeline_.schedule_and_wait(task)) {
            roc_panic("receiver decoder node: can't remove pipeline slot");
        }
    }

    // Then wait until processing task is fully completed, before
    // proceeding to its destruction.
    context().control_loop().wait(processing_task_);
}

bool ReceiverDecoder::is_valid() {
    return valid_;
}

bool ReceiverDecoder::activate(address::Interface iface, address::Protocol proto) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogInfo, "receiver decoder node: activating %s interface with protocol %s",
            address::interface_to_str(iface), address::proto_to_str(proto));

    if (endpoint_readers_[iface] || endpoint_writers_[iface]) {
        roc_log(LogError,
                "receiver decoder node:"
                " can't activate %s interface: interface already activated",
                address::interface_to_str(iface));
        return false;
    }

    endpoint_queues_[iface].reset(new (endpoint_queues_[iface]) packet::ConcurrentQueue(
        packet::ConcurrentQueue::NonBlocking));

    pipeline::ReceiverLoop::Tasks::AddEndpoint endpoint_task(
        slot_, iface, proto, bind_address_, endpoint_queues_[iface].get());
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError,
                "receiver decoder node:"
                " can't activate %s interface: can't add endpoint to pipeline",
                address::interface_to_str(iface));
        return false;
    }

    if (iface == address::Iface_AudioControl) {
        endpoint_readers_[iface] = endpoint_queues_[iface].get();
    }
    endpoint_writers_[iface] = endpoint_task.get_inbound_writer();

    return true;
}

bool ReceiverDecoder::get_metrics(slot_metrics_func_t slot_metrics_func,
                                  void* slot_metrics_arg,
                                  party_metrics_func_t party_metrics_func,
                                  size_t* party_metrics_size,
                                  void* party_metrics_arg) {
    core::Mutex::Lock lock(mutex_);

    roc_panic_if_not(is_valid());

    roc_panic_if(!slot_metrics_func);
    roc_panic_if(!party_metrics_func);

    if (party_metrics_size) {
        if (!party_metrics_.resize(*party_metrics_size)) {
            roc_log(LogError,
                    "receiver decoder node:"
                    " can't get metrics: can't allocate buffer");
            return false;
        }
    }

    pipeline::ReceiverLoop::Tasks::QuerySlot task(
        slot_, slot_metrics_, party_metrics_.data(), party_metrics_size);
    if (!pipeline_.schedule_and_wait(task)) {
        roc_log(LogError,
                "receiver decoder node:"
                " can't get metrics: operation failed");
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

status::StatusCode ReceiverDecoder::write_packet(address::Interface iface,
                                                 const packet::PacketPtr& packet) {
    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    packet::IWriter* writer = endpoint_writers_[iface];
    if (!writer) {
        roc_log(LogError,
                "receiver decoder node:"
                " can't write to %s interface: interface not activated",
                address::interface_to_str(iface));
        // TODO(gh-183): return StatusNotFound
        return status::StatusUnknown;
    }

    return writer->write(packet);
}

status::StatusCode ReceiverDecoder::read_packet(address::Interface iface,
                                                packet::PacketPtr& packet) {
    roc_panic_if_not(is_valid());

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    packet::IReader* reader = endpoint_readers_[iface];
    if (!reader) {
        if (!endpoint_writers_[iface]) {
            roc_log(LogError,
                    "receiver decoder node:"
                    " can't read from %s interface: interface not activated",
                    address::interface_to_str(iface));
            // TODO(gh-183): return StatusNotFound
            return status::StatusNoData;
        } else {
            roc_log(LogError,
                    "sender encoder node:"
                    " can't read from %s interface: interface doesn't support reading",
                    address::interface_to_str(iface));
            // TODO(gh-183): return StatusBadOperation
            return status::StatusNoData;
        }
    }

    return reader->read(packet);
}

sndio::ISource& ReceiverDecoder::source() {
    roc_panic_if_not(is_valid());

    return pipeline_.source();
}

void ReceiverDecoder::schedule_task_processing(pipeline::PipelineLoop&,
                                               core::nanoseconds_t deadline) {
    context().control_loop().schedule_at(processing_task_, deadline, NULL);
}

void ReceiverDecoder::cancel_task_processing(pipeline::PipelineLoop&) {
    context().control_loop().async_cancel(processing_task_);
}

} // namespace node
} // namespace roc
