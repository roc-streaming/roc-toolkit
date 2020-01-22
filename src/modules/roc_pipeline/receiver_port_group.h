/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_port_group.h
//! @brief Receiver port group.

#ifndef ROC_PIPELINE_RECEIVER_PORT_GROUP_H_
#define ROC_PIPELINE_RECEIVER_PORT_GROUP_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_audio/mixer.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_pipeline/receiver_port.h"
#include "roc_pipeline/receiver_session_group.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

//! Receiver port group.
class ReceiverPortGroup : public core::RefCnt<ReceiverPortGroup>, public core::ListNode {
public:
    //! Initialize.
    ReceiverPortGroup(const ReceiverConfig& receiver_config,
                      ReceiverState& receiver_state,
                      audio::Mixer& mixer,
                      const fec::CodecMap& codec_map,
                      const rtp::FormatMap& format_map,
                      packet::PacketPool& packet_pool,
                      core::BufferPool<uint8_t>& byte_buffer_pool,
                      core::BufferPool<audio::sample_t>& sample_buffer_pool,
                      core::IAllocator& allocator);

    //! Add port to the group.
    packet::IWriter* add_port(address::EndpointProtocol proto);

    //! Update packet queues and sessions.
    void update(packet::timestamp_t timestamp);

    //! Get number of alive sessions.
    size_t num_sessions() const;

private:
    friend class core::RefCnt<ReceiverPortGroup>;

    void destroy();

    core::IAllocator& allocator_;

    const rtp::FormatMap& format_map_;

    ReceiverState& receiver_state_;
    ReceiverSessionGroup session_group_;

    core::List<ReceiverPort> ports_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_PORT_GROUP_H_
