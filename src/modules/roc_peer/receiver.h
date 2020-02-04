/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_peer/receiver.h
//! @brief Receiver peer.

#ifndef ROC_PEER_RECEIVER_H_
#define ROC_PEER_RECEIVER_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_type.h"
#include "roc_address/socket_addr.h"
#include "roc_core/array.h"
#include "roc_core/mutex.h"
#include "roc_peer/basic_peer.h"
#include "roc_peer/context.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace peer {

//! Receiver peer.
class Receiver : public BasicPeer {
public:
    //! Initialize.
    Receiver(Context& context, const pipeline::ReceiverConfig& pipeline_config);

    //! Deinitialize.
    ~Receiver();

    //! Check if successfully constructed.
    bool valid();

    //! Bind peer to local endpoint.
    bool bind(address::EndpointType type,
              address::EndpointProtocol proto,
              address::SocketAddr& address);

    //! Get receiver source.
    sndio::ISource& source();

private:
    core::Mutex mutex_;

    rtp::FormatMap format_map_;

    pipeline::ReceiverSource pipeline_;
    pipeline::ReceiverSource::EndpointSetHandle endpoint_set_;

    core::Array<netio::EventLoop::PortHandle, 2> ports_;
};

} // namespace peer
} // namespace roc

#endif // ROC_PEER_RECEIVER_H_
