/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_node/sender.h
//! @brief Sender node.

#ifndef ROC_NODE_SENDER_H_
#define ROC_NODE_SENDER_H_

#include "roc_address/endpoint_uri.h"
#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/allocation_policy.h"
#include "roc_core/hashmap.h"
#include "roc_core/mutex.h"
#include "roc_core/pool.h"
#include "roc_core/ref_counted.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"
#include "roc_node/context.h"
#include "roc_node/node.h"
#include "roc_packet/iwriter.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/sender_loop.h"

namespace roc {
namespace node {

//! Sender node.
class Sender : public Node, private pipeline::IPipelineTaskScheduler {
public:
    //! Slot index.
    typedef uint64_t slot_index_t;

    //! Initialize.
    Sender(Context& context, const pipeline::SenderConfig& pipeline_config);

    //! Deinitialize.
    ~Sender();

    //! Check if successfully constructed.
    bool is_valid() const;

    //! Set interface config.
    bool configure(slot_index_t slot_index,
                   address::Interface iface,
                   const netio::UdpSenderConfig& config);

    //! Connect to remote endpoint.
    bool connect(slot_index_t slot_index,
                 address::Interface iface,
                 const address::EndpointUri& uri);

    //! Remove slot.
    bool unlink(slot_index_t slot_index);

    //! Get slot metrics.
    //! @remarks
    //!  Metrics are written into provided arguments.
    bool get_metrics(slot_index_t slot_index,
                     pipeline::SenderSlotMetrics& slot_metrics,
                     pipeline::SenderSessionMetrics& sess_metrics);

    //! Check if there are incomplete or broken slots.
    bool has_incomplete();

    //! Check if there are broken slots.
    bool has_broken();

    //! Get sender sink.
    sndio::ISink& sink();

private:
    struct Port {
        netio::UdpSenderConfig config;
        netio::UdpSenderConfig orig_config;
        netio::NetworkLoop::PortHandle handle;
        packet::IWriter* writer;

        Port()
            : handle(NULL)
            , writer(NULL) {
        }
    };

    struct Slot : core::RefCounted<Slot, core::PoolAllocation>, core::HashmapNode {
        const slot_index_t index;
        pipeline::SenderLoop::SlotHandle handle;
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

    bool check_compatibility_(address::Interface iface, const address::EndpointUri& uri);
    void update_compatibility_(address::Interface iface, const address::EndpointUri& uri);

    core::SharedPtr<Slot> get_slot_(slot_index_t slot_index, bool auto_create);
    void cleanup_slot_(Slot& slot);
    void break_slot_(Slot& slot);

    Port&
    select_outgoing_port_(Slot& slot, address::Interface, address::AddrFamily family);
    bool setup_outgoing_port_(Port& port,
                              address::Interface iface,
                              address::AddrFamily family);

    virtual void schedule_task_processing(pipeline::PipelineLoop&,
                                          core::nanoseconds_t delay);
    virtual void cancel_task_processing(pipeline::PipelineLoop&);

    core::Mutex mutex_;

    pipeline::SenderLoop pipeline_;
    ctl::ControlLoop::Tasks::PipelineProcessing processing_task_;

    core::Pool<Slot> slot_pool_;
    core::Hashmap<Slot> slot_map_;

    bool used_interfaces_[address::Iface_Max];
    address::Protocol used_protocols_[address::Iface_Max];

    bool valid_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_SENDER_H_
