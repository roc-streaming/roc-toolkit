/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_peer/receiver.h
//! @brief Receiver peer.

#ifndef ROC_PEER_RECEIVER_H_
#define ROC_PEER_RECEIVER_H_

#include "roc_address/endpoint_uri.h"
#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/hashmap.h"
#include "roc_core/mutex.h"
#include "roc_core/pool.h"
#include "roc_core/ref_counted.h"
#include "roc_ctl/control_loop.h"
#include "roc_peer/basic_peer.h"
#include "roc_peer/context.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/receiver_loop.h"

namespace roc {
namespace peer {

//! Receiver peer.
class Receiver : public BasicPeer, private pipeline::IPipelineTaskScheduler {
public:
    //! Initialize.
    Receiver(Context& context, const pipeline::ReceiverConfig& pipeline_config);

    //! Deinitialize.
    ~Receiver();

    //! Check if successfully constructed.
    bool is_valid();

    //! Set interface config.
    bool configure(size_t slot_index,
                   address::Interface iface,
                   const netio::UdpReceiverConfig& config);

    //! Bind peer to local endpoint.
    bool bind(size_t slot_index, address::Interface iface, address::EndpointUri& uri);

    //! Remove slot.
    bool unlink(size_t slot_index);

    //! Check if there are broken slots.
    bool has_broken();

    //! Get receiver source.
    sndio::ISource& source();

private:
    struct Port {
        netio::UdpReceiverConfig config;
        netio::NetworkLoop::PortHandle handle;

        Port()
            : handle(NULL) {
        }
    };

    struct Slot : core::RefCounted<Slot, core::PoolAllocation>, core::HashmapNode {
        const size_t index;
        pipeline::ReceiverLoop::SlotHandle handle;
        Port ports[address::Iface_Max];
        bool broken;

        Slot(core::IPool& pool, size_t index, pipeline::SenderLoop::SlotHandle handle)
            : core::RefCounted<Slot, core::PoolAllocation>(pool)
            , index(index)
            , handle(handle)
            , broken(false) {
        }

        size_t key() const {
            return index;
        }

        static core::hashsum_t key_hash(size_t index) {
            return core::hashsum_int(index);
        }

        static bool key_equal(size_t index1, size_t index2) {
            return index1 == index2;
        }
    };

    bool check_compatibility_(address::Interface iface, const address::EndpointUri& uri);
    void update_compatibility_(address::Interface iface, const address::EndpointUri& uri);

    core::SharedPtr<Slot> get_slot_(size_t slot_index, bool auto_create);
    void cleanup_slot_(Slot& slot);
    void break_slot_(Slot& slot);

    virtual void schedule_task_processing(pipeline::PipelineLoop&,
                                          core::nanoseconds_t delay);
    virtual void cancel_task_processing(pipeline::PipelineLoop&);

    core::Mutex mutex_;

    pipeline::ReceiverLoop pipeline_;
    ctl::ControlLoop::Tasks::PipelineProcessing processing_task_;

    core::Pool<Slot> slot_pool_;
    core::Hashmap<Slot> slot_map_;

    bool used_interfaces_[address::Iface_Max];
    address::Protocol used_protocols_[address::Iface_Max];

    bool valid_;
};

} // namespace peer
} // namespace roc

#endif // ROC_PEER_RECEIVER_H_
