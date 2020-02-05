/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_peer/sender.h
//! @brief Sender peer.

#ifndef ROC_PEER_SENDER_H_
#define ROC_PEER_SENDER_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_type.h"
#include "roc_address/socket_addr.h"
#include "roc_core/mutex.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/iwriter.h"
#include "roc_peer/basic_peer.h"
#include "roc_peer/context.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace peer {

//! Sender peer.
class Sender : public BasicPeer {
public:
    //! Initialize.
    Sender(Context& context, const pipeline::SenderConfig& pipeline_config);

    //! Deinitialize.
    ~Sender();

    //! Check if successfully constructed.
    bool valid() const;

    //! Enable or disable traffic to broadcast addresses.
    bool set_broadcast_enabled(bool);

    //! Bind peer to local endpoint.
    bool bind(address::SocketAddr& addr);

    //! Connect peer to remote endpoint.
    bool connect(address::EndpointType type,
                 address::EndpointProtocol proto,
                 const address::SocketAddr& address);

    //! Check if all necessary bind and connect calls were made.
    bool is_ready() const;

    //! Get sender sink.y
    sndio::ISink& sink();

private:
    core::Mutex mutex_;

    rtp::FormatMap format_map_;

    pipeline::SenderSink pipeline_;
    pipeline::SenderSink::EndpointSetHandle endpoint_set_;

    pipeline::SenderSink::EndpointHandle source_endpoint_;
    pipeline::SenderSink::EndpointHandle repair_endpoint_;

    netio::UdpSenderConfig udp_config_;
    netio::EventLoop::PortHandle udp_port_;
    packet::IWriter* udp_writer_;
};

} // namespace peer
} // namespace roc

#endif // ROC_PEER_SENDER_H_
