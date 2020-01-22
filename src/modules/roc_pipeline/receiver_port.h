/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_port.h
//! @brief Receiver port pipeline.

#ifndef ROC_PIPELINE_RECEIVER_PORT_H_
#define ROC_PIPELINE_RECEIVER_PORT_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_core/iallocator.h"
#include "roc_core/list_node.h"
#include "roc_core/mutex.h"
#include "roc_core/refcnt.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/iparser.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/queue.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/receiver_session_group.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace pipeline {

//! Receiver port pipeline.
//! @remarks
//!  Created at the receiver side for every listened port.
class ReceiverPort : public packet::IWriter,
                     public core::RefCnt<ReceiverPort>,
                     public core::ListNode {
public:
    //! Initialize.
    ReceiverPort(address::EndpointProtocol proto,
                 ReceiverState& receiver_state,
                 ReceiverSessionGroup& session_group,
                 const rtp::FormatMap& format_map,
                 core::IAllocator& allocator);

    //! Check if the port pipeline was succefully constructed.
    bool valid() const;

    //! Handle packet.
    //! Called outside of pipeline from any thread, typically from netio thread.
    virtual void write(const packet::PacketPtr& packet);

    //! Flush queued packets.
    //! Called from pipeline thread.
    void flush_packets();

private:
    friend class core::RefCnt<ReceiverPort>;

    void destroy();

    packet::Queue* get_read_queue_();

    core::IAllocator& allocator_;

    ReceiverState& receiver_state_;
    ReceiverSessionGroup& session_group_;

    packet::IParser* parser_;

    core::ScopedPtr<rtp::Parser> rtp_parser_;
    core::ScopedPtr<packet::IParser> fec_parser_;

    core::Mutex queue_mutex_;
    packet::Queue queues_[2];
    size_t cur_queue_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_PORT_H_
