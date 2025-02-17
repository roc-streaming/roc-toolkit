/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_node/receiver.h
//! @brief Receiver node.

#ifndef ROC_NODE_RECEIVER_H_
#define ROC_NODE_RECEIVER_H_

#include "roc_address/interface.h"
#include "roc_address/network_uri.h"
#include "roc_address/protocol.h"
#include "roc_core/attributes.h"
#include "roc_core/hashmap.h"
#include "roc_core/mutex.h"
#include "roc_core/ref_counted.h"
#include "roc_core/slab_pool.h"
#include "roc_core/stddefs.h"
#include "roc_ctl/control_loop.h"
#include "roc_node/context.h"
#include "roc_node/node.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/receiver_loop.h"

namespace roc {
namespace node {

//! Receiver node.
class Receiver : public Node, private pipeline::IPipelineTaskScheduler {
public:
    //! Slot index.
    typedef uint64_t slot_index_t;

    //! Initialize.
    Receiver(Context& context, const pipeline::ReceiverSourceConfig& pipeline_config);

    //! Deinitialize.
    ~Receiver();

    //! Check if the node was successfully constructed.
    status::StatusCode init_status() const;

    //! Set interface config.
    ROC_ATTR_NODISCARD bool configure(slot_index_t slot_index,
                                      address::Interface iface,
                                      const netio::UdpConfig& config);

    //! Bind to local endpoint.
    ROC_ATTR_NODISCARD bool
    bind(slot_index_t slot_index, address::Interface iface, address::NetworkUri& uri);

    //! Remove slot.
    ROC_ATTR_NODISCARD bool unlink(slot_index_t slot_index);

    //! Callback for slot metrics.
    typedef void (*slot_metrics_func_t)(const pipeline::ReceiverSlotMetrics& slot_metrics,
                                        void* slot_arg);

    //! Callback for participant metrics.
    typedef void (*party_metrics_func_t)(
        const pipeline::ReceiverParticipantMetrics& party_metrics,
        size_t party_index,
        void* party_arg);

    //! Get metrics.
    ROC_ATTR_NODISCARD bool get_metrics(slot_index_t slot_index,
                                        slot_metrics_func_t slot_metrics_func,
                                        void* slot_metrics_arg,
                                        party_metrics_func_t party_metrics_func,
                                        size_t* party_metrics_size,
                                        void* party_metrics_arg);

    //! Check if there are broken slots.
    bool has_broken_slots();

    //! Read frame.
    //! @remarks
    //!  Performs necessary checks and allocations on top of ISource::read(),
    //!  used when working with raw byte buffers instead of Frame objects.
    ROC_ATTR_NODISCARD status::StatusCode read_frame(void* bytes, size_t n_bytes);

    //! Get receiver source.
    sndio::ISource& source();

private:
    struct Port {
        netio::UdpConfig config;
        netio::NetworkLoop::PortHandle handle;

        Port()
            : handle(NULL) {
        }
    };

    struct Slot : core::RefCounted<Slot, core::PoolAllocation>, core::HashmapNode<> {
        const slot_index_t index;
        pipeline::ReceiverLoop::SlotHandle handle;
        Port ports[address::Iface_Max];
        bool broken;

        Slot(core::IPool& pool,
             slot_index_t index,
             pipeline::SenderLoop::SlotHandle handle)
            : core::RefCounted<Slot, core::PoolAllocation>(pool)
            , index(index)
            , handle(handle)
            , broken(false) {
        }

        slot_index_t key() const {
            return index;
        }

        static core::hashsum_t key_hash(slot_index_t index) {
            return core::hashsum_int(index);
        }

        static bool key_equal(slot_index_t index1, slot_index_t index2) {
            return index1 == index2;
        }
    };

    core::SharedPtr<Slot> get_slot_(slot_index_t slot_index, bool auto_create);
    void cleanup_slot_(Slot& slot);
    void break_slot_(Slot& slot);

    virtual void schedule_task_processing(pipeline::PipelineLoop&,
                                          core::nanoseconds_t delay);
    virtual void cancel_task_processing(pipeline::PipelineLoop&);

    core::Mutex control_mutex_;

    pipeline::ReceiverLoop pipeline_;
    ctl::ControlLoop::Tasks::PipelineProcessing processing_task_;

    core::SlabPool<Slot> slot_pool_;
    core::Hashmap<Slot> slot_map_;

    pipeline::ReceiverSlotMetrics slot_metrics_;
    core::Array<pipeline::ReceiverParticipantMetrics, 8> party_metrics_;

    core::Mutex frame_mutex_;

    audio::FrameFactory frame_factory_;
    audio::FramePtr frame_;
    audio::SampleSpec sample_spec_;

    status::StatusCode init_status_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_RECEIVER_H_
