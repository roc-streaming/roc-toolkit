/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_node/sender_encoder.h"
#include "roc_address/interface.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_pipeline/metrics.h"
#include "roc_status/code_to_str.h"

namespace roc {
namespace node {

SenderEncoder::SenderEncoder(Context& context,
                             const pipeline::SenderSinkConfig& pipeline_config)
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
    , slot_(NULL)
    , processing_task_(pipeline_)
    , packet_factory_(context.packet_pool(), context.packet_buffer_pool())
    , frame_factory_(context.frame_pool(), context.frame_buffer_pool())
    , init_status_(status::NoStatus) {
    roc_log(LogDebug, "sender encoder node: initializing");

    if ((init_status_ = pipeline_.init_status()) != status::StatusOK) {
        roc_log(LogError, "sender encoder node: failed to construct pipeline: status=%s",
                status::code_to_str(pipeline_.init_status()));
        return;
    }

    sample_spec_ = pipeline_.sink().sample_spec();

    pipeline::SenderSlotConfig slot_config;

    pipeline::SenderLoop::Tasks::CreateSlot slot_task(slot_config);
    if (!pipeline_.schedule_and_wait(slot_task)) {
        roc_log(LogError, "sender encoder node: failed to create slot");
        // TODO(gh-183): forward status (control ops)
        return;
    }

    slot_ = slot_task.get_handle();
    if (!slot_) {
        roc_log(LogError, "sender encoder node: failed to create slot");
        // TODO(gh-183): forward status (control ops)
        return;
    }

    init_status_ = status::StatusOK;
}

SenderEncoder::~SenderEncoder() {
    roc_log(LogDebug, "sender encoder node: deinitializing");

    if (slot_) {
        // First remove slot. This may involve usage of processing task.
        pipeline::SenderLoop::Tasks::DeleteSlot task(slot_);
        if (!pipeline_.schedule_and_wait(task)) {
            roc_panic("sender encoder node: can't remove pipeline slot");
        }
    }

    // Then wait until processing task is fully completed, before
    // proceeding to its destruction.
    context().control_loop().wait(processing_task_);
}

status::StatusCode SenderEncoder::init_status() const {
    return init_status_;
}

packet::PacketFactory& SenderEncoder::packet_factory() {
    return packet_factory_;
}

bool SenderEncoder::activate(address::Interface iface, address::Protocol proto) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_log(LogInfo, "sender encoder node: activating %s interface with protocol %s",
            address::interface_to_str(iface), address::proto_to_str(proto));

    if (endpoint_readers_[iface] || endpoint_writers_[iface]) {
        roc_log(LogError,
                "sender encoder node:"
                " can't activate %s interface: interface already activated",
                address::interface_to_str(iface));
        return false;
    }

    endpoint_queues_[iface].reset(new (endpoint_queues_[iface]) packet::ConcurrentQueue(
        packet::ConcurrentQueue::NonBlocking));

    pipeline::SenderLoop::Tasks::AddEndpoint endpoint_task(
        slot_, iface, proto, dest_address_, *endpoint_queues_[iface]);
    if (!pipeline_.schedule_and_wait(endpoint_task)) {
        roc_log(LogError,
                "sender encoder node:"
                " can't activate %s interface: can't add endpoint to pipeline",
                address::interface_to_str(iface));
        return false;
    }

    endpoint_readers_[iface] = endpoint_queues_[iface].get();

    if (iface == address::Iface_AudioControl) {
        endpoint_writers_[iface] = endpoint_task.get_inbound_writer();
    }

    return true;
}

bool SenderEncoder::get_metrics(slot_metrics_func_t slot_metrics_func,
                                void* slot_metrics_arg,
                                party_metrics_func_t party_metrics_func,
                                void* party_metrics_arg) {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(!slot_metrics_func);
    roc_panic_if(!party_metrics_func);

    pipeline::SenderSlotMetrics slot_metrics;
    pipeline::SenderParticipantMetrics party_metrics;
    size_t party_metrics_size = 1;

    pipeline::SenderLoop::Tasks::QuerySlot task(slot_, slot_metrics, &party_metrics,
                                                &party_metrics_size);
    if (!pipeline_.schedule_and_wait(task)) {
        roc_log(LogError,
                "sender encoder node:"
                " can't get metrics: operation failed");
        return false;
    }

    if (slot_metrics_arg) {
        slot_metrics_func(slot_metrics, slot_metrics_arg);
    }

    if (party_metrics_arg) {
        party_metrics_func(party_metrics, 0, party_metrics_arg);
    }

    return true;
}

bool SenderEncoder::is_complete() {
    core::Mutex::Lock lock(control_mutex_);

    roc_panic_if(init_status_ != status::StatusOK);

    pipeline::SenderSlotMetrics slot_metrics;
    pipeline::SenderLoop::Tasks::QuerySlot task(slot_, slot_metrics, NULL, NULL);
    if (!pipeline_.schedule_and_wait(task)) {
        return false;
    }

    return slot_metrics.is_complete;
}

status::StatusCode
SenderEncoder::read_packet(address::Interface iface, void* bytes, size_t* n_bytes) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_panic_if(!bytes);
    roc_panic_if(n_bytes == 0);

    packet::IReader* reader = endpoint_readers_[iface];
    if (!reader) {
        roc_log(LogError,
                "sender encoder node:"
                " can't read from %s interface: interface not activated",
                address::interface_to_str(iface));
        return status::StatusBadInterface;
    }

    packet::PacketPtr packet;
    const status::StatusCode code = reader->read(packet, packet::ModeFetch);
    if (code != status::StatusOK) {
        return code;
    }

    if (*n_bytes < packet->buffer().size()) {
        roc_log(LogError,
                "sender encoder node:"
                " not enough space in provided packet:"
                " provided=%lu needed=%lu",
                (unsigned long)n_bytes, (unsigned long)packet->buffer().size());
        return status::StatusBadBuffer;
    }

    memcpy(bytes, packet->buffer().data(), packet->buffer().size());
    *n_bytes = packet->buffer().size();

    return status::StatusOK;
}

status::StatusCode
SenderEncoder::write_packet(address::Interface iface, const void* bytes, size_t n_bytes) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if(iface < 0);
    roc_panic_if(iface >= (int)address::Iface_Max);

    roc_panic_if(!bytes);
    roc_panic_if(n_bytes == 0);

    const core::nanoseconds_t capture_ts = core::timestamp(core::ClockUnix);

    if (n_bytes > packet_factory_.packet_buffer_size()) {
        roc_log(LogError,
                "sender encoder node:"
                " provided packet exceeds maximum packet size (see roc_context_config):"
                " provided=%lu maximum=%lu",
                (unsigned long)n_bytes,
                (unsigned long)packet_factory_.packet_buffer_size());
        return status::StatusBadBuffer;
    }

    core::Slice<uint8_t> buffer = packet_factory_.new_packet_buffer();
    if (!buffer) {
        roc_log(LogError, "sender encoder node: can't allocate buffer");
        return status::StatusNoMem;
    }

    buffer.reslice(0, n_bytes);
    memcpy(buffer.data(), bytes, n_bytes);

    packet::PacketPtr packet = packet_factory_.new_packet();
    if (!packet) {
        roc_log(LogError, "sender encoder node: can't allocate packet");
        return status::StatusNoMem;
    }

    packet->add_flags(packet::Packet::FlagUDP);
    packet->udp()->receive_timestamp = capture_ts;
    packet->set_buffer(buffer);

    packet::IWriter* writer = endpoint_writers_[iface];
    if (!writer) {
        if (!endpoint_readers_[iface]) {
            roc_log(LogError,
                    "sender encoder node:"
                    " can't write to %s interface: interface not activated",
                    address::interface_to_str(iface));
            return status::StatusBadInterface;
        } else {
            roc_log(LogError,
                    "sender encoder node:"
                    " can't write to %s interface: interface doesn't support writing",
                    address::interface_to_str(iface));
            return status::StatusBadOperation;
        }
    }

    return writer->write(packet);
}

status::StatusCode SenderEncoder::write_frame(const void* bytes, size_t n_bytes) {
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

sndio::ISink& SenderEncoder::sink() {
    roc_panic_if(init_status_ != status::StatusOK);

    return pipeline_.sink();
}

void SenderEncoder::schedule_task_processing(pipeline::PipelineLoop&,
                                             core::nanoseconds_t deadline) {
    context().control_loop().schedule_at(processing_task_, deadline, NULL);
}

void SenderEncoder::cancel_task_processing(pipeline::PipelineLoop&) {
    context().control_loop().async_cancel(processing_task_);
}

} // namespace node
} // namespace roc
