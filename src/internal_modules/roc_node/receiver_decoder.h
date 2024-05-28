/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_node/receiver_decoder.h
//! @brief Receiver decoder node.

#ifndef ROC_NODE_RECEIVER_DECODER_H_
#define ROC_NODE_RECEIVER_DECODER_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/attributes.h"
#include "roc_core/mutex.h"
#include "roc_node/context.h"
#include "roc_node/node.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/receiver_loop.h"
#include "roc_status/status_code.h"

namespace roc {
namespace node {

//! Receiver decoder node.
class ReceiverDecoder : public Node, private pipeline::IPipelineTaskScheduler {
public:
    //! Initialize.
    ReceiverDecoder(Context& context,
                    const pipeline::ReceiverSourceConfig& pipeline_config);

    //! Deinitialize.
    ~ReceiverDecoder();

    //! Check if the node was successfully constructed.
    status::StatusCode init_status() const;

    //! Activate interface.
    ROC_ATTR_NODISCARD bool activate(address::Interface iface, address::Protocol proto);

    //! Callback for slot metrics.
    typedef void (*slot_metrics_func_t)(const pipeline::ReceiverSlotMetrics& slot_metrics,
                                        void* slot_arg);

    //! Callback for participant metrics.
    typedef void (*party_metrics_func_t)(
        const pipeline::ReceiverParticipantMetrics& party_metrics,
        size_t party_index,
        void* party_arg);

    //! Get metrics.
    ROC_ATTR_NODISCARD bool get_metrics(slot_metrics_func_t slot_metrics_func,
                                        void* slot_metrics_arg,
                                        party_metrics_func_t party_metrics_func,
                                        void* party_metrics_arg);

    //! Write packet for decoding.
    ROC_ATTR_NODISCARD status::StatusCode
    write_packet(address::Interface iface, const void* bytes, size_t n_bytes);

    //! Read encoded packet.
    //! @note
    //!  Typically used to generate control packets with feedback for sender.
    ROC_ATTR_NODISCARD status::StatusCode
    read_packet(address::Interface iface, void* bytes, size_t* n_bytes);

    //! Read frame into byte buffer.
    //! @remarks
    //!  Performs necessary checks and allocations on top of ISource::read(),
    //!  needed when working with byte buffers instead of Frame objects.
    ROC_ATTR_NODISCARD status::StatusCode read_frame(void* bytes, size_t n_bytes);

    //! Source for reading decoded frames.
    sndio::ISource& source();

private:
    virtual void schedule_task_processing(pipeline::PipelineLoop&,
                                          core::nanoseconds_t delay);
    virtual void cancel_task_processing(pipeline::PipelineLoop&);

    core::Mutex control_mutex_;

    address::SocketAddr bind_address_;

    core::Optional<packet::ConcurrentQueue> endpoint_queues_[address::Iface_Max];
    core::Atomic<packet::IReader*> endpoint_readers_[address::Iface_Max];
    core::Atomic<packet::IWriter*> endpoint_writers_[address::Iface_Max];

    pipeline::ReceiverLoop pipeline_;
    pipeline::ReceiverLoop::SlotHandle slot_;
    ctl::ControlLoop::Tasks::PipelineProcessing processing_task_;

    packet::PacketFactory packet_factory_;

    core::Mutex frame_mutex_;

    audio::FrameFactory frame_factory_;
    audio::FramePtr frame_;
    audio::SampleSpec sample_spec_;

    status::StatusCode init_status_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_RECEIVER_DECODER_H_
