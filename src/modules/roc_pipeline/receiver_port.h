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

#include "roc_core/iallocator.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_core/scoped_ptr.h"
#include "roc_packet/iparser.h"
#include "roc_pipeline/config.h"
#include "roc_rtp/format_map.h"
#include "roc_rtp/parser.h"

namespace roc {
namespace pipeline {

//! Receiver port pipeline.
//! @remarks
//!  Created at the receiver side for every listened port.
class ReceiverPort : public core::RefCnt<ReceiverPort>, public core::ListNode {
public:
    //! Initialize.
    ReceiverPort(const PortConfig& config,
                 const rtp::FormatMap& format_map,
                 core::IAllocator& allocator);

    //! Check if the port pipeline was succefully constructed.
    bool valid() const;

    //! Get port config.
    const PortConfig& config() const;

    //! Try to handle packet on this port.
    //! @returns
    //!  true if the packet is dedicated for this port
    bool handle(packet::Packet& packet);

private:
    friend class core::RefCnt<ReceiverPort>;

    void destroy();

    core::IAllocator& allocator_;

    const PortConfig config_;

    packet::IParser* parser_;

    core::ScopedPtr<rtp::Parser> rtp_parser_;
    core::ScopedPtr<packet::IParser> fec_parser_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_PORT_H_
