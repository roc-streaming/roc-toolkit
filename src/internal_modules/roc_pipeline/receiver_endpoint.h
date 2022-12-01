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
#include "roc_core/iallocator.h"
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

//! Receiver endpoint pipeline.
//! @remarks
//!  Created for every transport endpoint. Belongs to endpoint set.
//!  Passes packets to the session group of the endpoint set.
class ReceiverEndpoint
    : public core::RefCounted<ReceiverEndpoint, core::StandardAllocation>,
      public core::ListNode,
      private packet::IWriter {
    typedef core::RefCounted<ReceiverEndpoint, core::StandardAllocation> RefCounted;

public:
    //! Initialize.
    ReceiverEndpoint(address::Protocol proto,
                     ReceiverState& receiver_state,
                     ReceiverSessionGroup& session_group,
                     const rtp::FormatMap& format_map,
                     core::IAllocator& allocator);

    //! Check if the port pipeline was succefully constructed.
    bool valid() const;

    //! Get protocol.
    address::Protocol proto() const;

    //! Get endpoint writer.
    //! @remarks
    //!  Packets passed to this writer will be pulled by endpoint pipeline.
    //!  This writer is thread-safe and lock-free.
    //!  The writer is passed to netio thread.
    packet::IWriter& writer();

    //! Pull packets writter to endpoint writer.
    void pull_packets();

private:
    virtual void write(const packet::PacketPtr& packet);

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
