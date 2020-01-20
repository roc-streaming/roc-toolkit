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

#include "roc_address/socket_addr.h"
#include "roc_core/mutex.h"
#include "roc_core/scoped_ptr.h"
#include "roc_fec/codec_map.h"
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
    bool valid();

    //! Bind peer to local port.
    bool bind(address::SocketAddr& addr);

    //! Connect peer to remote port.
    bool connect(pipeline::PortType port_type, const pipeline::PortConfig& port_config);

    //! Get sender sink.
    sndio::ISink* sink();

private:
    bool set_source_port_(const pipeline::PortConfig& port_config);
    bool set_repair_port_(const pipeline::PortConfig& port_config);

    bool ensure_pipeline_();

    core::Mutex mutex_;

    const pipeline::SenderConfig pipeline_config_;

    fec::CodecMap codec_map_;
    rtp::FormatMap format_map_;

    pipeline::PortConfig source_port_;
    pipeline::PortConfig repair_port_;

    core::ScopedPtr<pipeline::SenderSink> pipeline_;
    packet::IWriter* udp_writer_;

    address::SocketAddr bind_address_;
};

} // namespace peer
} // namespace roc

#endif // ROC_PEER_SENDER_H_
