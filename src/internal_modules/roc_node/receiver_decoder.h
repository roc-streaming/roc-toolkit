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
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/receiver_loop.h"
#include "roc_status/status_code.h"

namespace roc {
namespace node {

//! Receiver decoder node.
class ReceiverDecoder : public Node, private pipeline::IPipelineTaskScheduler {
public:
    //! Initialize.
    ReceiverDecoder(Context& context, const pipeline::ReceiverConfig& pipeline_config);

    //! Deinitialize.
    ~ReceiverDecoder();

    //! Check if successfully constructed.
    bool is_valid();

    //! Activate interface.
    bool activate(address::Interface iface, address::Protocol proto);

    //! Callback for getting session metrics.
    typedef void (*sess_metrics_func_t)(
        const pipeline::ReceiverSessionMetrics& sess_metrics,
        size_t sess_index,
        void* sess_arg);

    //! Get metrics.
    //! @remarks
    //!  Metrics for slot are written into @p slot_metrics.
    //!  Metrics for each session are passed to @p sess_metrics_func.
    bool get_metrics(pipeline::ReceiverSlotMetrics& slot_metrics,
                     sess_metrics_func_t sess_metrics_func,
                     size_t* sess_metrics_size,
                     void* sess_metrics_arg);

    //! Write packet for decoding.
    ROC_ATTR_NODISCARD status::StatusCode write(address::Interface iface,
                                                const packet::PacketPtr& packet);

    //! Source for reading decoded frames.
    sndio::ISource& source();

private:
    virtual void schedule_task_processing(pipeline::PipelineLoop&,
                                          core::nanoseconds_t delay);
    virtual void cancel_task_processing(pipeline::PipelineLoop&);

    core::Mutex mutex_;

    address::SocketAddr dest_address_;

    core::Optional<packet::ConcurrentQueue> endpoint_queues_[address::Iface_Max];
    core::Atomic<packet::IWriter*> endpoint_writers_[address::Iface_Max];

    pipeline::ReceiverLoop pipeline_;
    pipeline::ReceiverLoop::SlotHandle slot_;
    ctl::ControlLoop::Tasks::PipelineProcessing processing_task_;

    core::Array<pipeline::ReceiverSessionMetrics, 8> sess_metrics_;

    bool valid_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_RECEIVER_DECODER_H_
