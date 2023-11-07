/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_endpoint.h
//! @brief Receiver endpoint pipeline.

#ifndef ROC_PIPELINE_RECEIVER_ENDPOINT_H_
#define ROC_PIPELINE_RECEIVER_ENDPOINT_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_core/iarena.h"
#include "roc_core/mpsc_queue.h"
#include "roc_core/optional.h"
#include "roc_core/ref_counted.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/iparser.h"
#include "roc_packet/iwriter.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_session_group.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_rtcp/parser.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace pipeline {

//! Receiver endpoint sub-pipeline.
//!
//! Contains:
//!  - a pipeline for processing packets from single network endpoint
//!  - a reference to session group to which packets are routed
class ReceiverEndpoint : public core::RefCounted<ReceiverEndpoint, core::ArenaAllocation>,
                         public core::ListNode,
                         private packet::IWriter {
public:
    //! Initialize.
    //!  - @p writer to handle packets received on netio thread.
    ReceiverEndpoint(address::Protocol proto,
                     ReceiverState& receiver_state,
                     ReceiverSessionGroup& session_group,
                     const rtp::FormatMap& format_map,
                     core::IArena& arena);

    //! Check if the port pipeline was succefully constructed.
    bool is_valid() const;

    //! Get protocol.
    address::Protocol proto() const;

    //! Get endpoint writer.
    //! @remarks
    //!  Packets passed to this writer will be pulled by endpoint pipeline.
    //!  This writer is thread-safe and lock-free.
    //!  The writer is passed to netio thread.
    packet::IWriter& writer();

    //! Pull packets writter to endpoint writer.
    ROC_ATTR_NODISCARD status::StatusCode pull_packets();

private:
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr& packet);

    const address::Protocol proto_;

    ReceiverState& receiver_state_;
    ReceiverSessionGroup& session_group_;

    packet::IParser* parser_;

    core::Optional<rtp::Parser> rtp_parser_;
    core::ScopedPtr<packet::IParser> fec_parser_;
    core::Optional<rtcp::Parser> rtcp_parser_;

    core::MpscQueue<packet::Packet> queue_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_ENDPOINT_H_
