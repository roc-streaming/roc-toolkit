/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_peer/sender.h
//! @brief Sender peer.

#ifndef ROC_PEER_SENDER_H_
#define ROC_PEER_SENDER_H_

#include "roc_address/endpoint_uri.h"
#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/mutex.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/iwriter.h"
#include "roc_peer/basic_peer.h"
#include "roc_peer/context.h"
#include "roc_pipeline/ipipeline_task_scheduler.h"
#include "roc_pipeline/sender_loop.h"

namespace roc {
namespace peer {

//! Sender peer.
class Sender : public BasicPeer, private pipeline::IPipelineTaskScheduler {
public:
    //! Initialize.
    Sender(Context& context, const pipeline::SenderConfig& pipeline_config);

    //! Deinitialize.
    ~Sender();

    //! Check if successfully constructed.
    bool valid() const;

    //! Set outgoing interface address.
    bool
    set_outgoing_address(size_t slot_index, address::Interface iface, const char* ip);

    //! Set reuseaddr option for given endpoint type.
    bool set_reuseaddr(size_t slot_index, address::Interface iface, bool enabled);

    //! Connect peer to remote endpoint.
    bool
    connect(size_t slot_index, address::Interface iface, const address::EndpointUri& uri);

    //! Check if all necessary bind and connect calls were made.
    bool is_ready();

    //! Get sender sink.y
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

    struct Slot {
        pipeline::SenderLoop::SlotHandle slot;
        Port ports[address::Iface_Max];

        Slot()
            : slot(NULL) {
        }
    };

    bool check_compatibility_(address::Interface iface, const address::EndpointUri& uri);
    void update_compatibility_(address::Interface iface, const address::EndpointUri& uri);

    Slot* get_slot_(size_t slot_index);
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

    core::Array<Slot, 8> slots_;

    bool used_interfaces_[address::Iface_Max];
    address::Protocol used_protocols_[address::Iface_Max];

    bool valid_;
};

} // namespace peer
} // namespace roc

#endif // ROC_PEER_SENDER_H_
