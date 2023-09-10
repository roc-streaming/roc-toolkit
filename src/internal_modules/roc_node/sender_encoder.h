/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_node/sender_encoder.h
//! @brief Sender encoder node.

#ifndef ROC_NODE_SENDER_ENCODER_H_
#define ROC_NODE_SENDER_ENCODER_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_address/socket_addr.h"
#include "roc_core/atomic.h"
#include "roc_core/mutex.h"
#include "roc_core/optional.h"
#include "roc_node/context.h"
#include "roc_node/node.h"
#include "roc_packet/concurrent_queue.h"
#include "roc_packet/ireader.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/sender_loop.h"

namespace roc {
namespace node {

//! Sender encoder node.
class SenderEncoder : public Node, private pipeline::IPipelineTaskScheduler {
public:
    //! Initialize.
    SenderEncoder(Context& context, const pipeline::SenderConfig& pipeline_config);

    //! Deinitialize.
    ~SenderEncoder();

    //! Check if successfully constructed.
    bool is_valid() const;

    //! Activate interface.
    bool activate(address::Interface iface, address::Protocol proto);

    //! Get metrics.
    bool get_metrics(pipeline::SenderSlotMetrics& slot_metrics,
                     pipeline::SenderSessionMetrics& sess_metrics);

    //! Check if everything is connected.
    bool is_complete();

    //! Read encoded packet.
    bool read(address::Interface iface, packet::PacketPtr& packet);

    //! Sink for writing frames for encoding.
    sndio::ISink& sink();

private:
    virtual void schedule_task_processing(pipeline::PipelineLoop&,
                                          core::nanoseconds_t delay);
    virtual void cancel_task_processing(pipeline::PipelineLoop&);

    core::Mutex mutex_;

    address::SocketAddr address_;

    core::Optional<packet::ConcurrentQueue> endpoint_queues_[address::Iface_Max];
    core::Atomic<packet::IReader*> endpoint_readers_[address::Iface_Max];

    pipeline::SenderLoop pipeline_;
    pipeline::SenderLoop::SlotHandle slot_;
    ctl::ControlLoop::Tasks::PipelineProcessing processing_task_;

    bool valid_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_SENDER_ENCODER_H_
