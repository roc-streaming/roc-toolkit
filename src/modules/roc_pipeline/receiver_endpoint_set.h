/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_endpoint_set.h
//! @brief Receiver endpoint set.

#ifndef ROC_PIPELINE_RECEIVER_ENDPOINT_SET_H_
#define ROC_PIPELINE_RECEIVER_ENDPOINT_SET_H_

#include "roc_address/endpoint_protocol.h"
#include "roc_address/endpoint_type.h"
#include "roc_audio/mixer.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/refcnt.h"
#include "roc_pipeline/receiver_endpoint.h"
#include "roc_pipeline/receiver_session_group.h"
#include "roc_pipeline/receiver_state.h"
#include "roc_rtp/format_map.h"

namespace roc {
namespace pipeline {

//! Receiver endpoint set.
//! @remarks
//!  Contains one or seevral related endpoint pipelines and a
//!  session group shared by them.
class ReceiverEndpointSet : public core::RefCnt<ReceiverEndpointSet>,
                            public core::ListNode {
public:
    //! Initialize.
    ReceiverEndpointSet(const ReceiverConfig& receiver_config,
                        ReceiverState& receiver_state,
                        audio::Mixer& mixer,
                        const rtp::FormatMap& format_map,
                        packet::PacketPool& packet_pool,
                        core::BufferPool<uint8_t>& byte_buffer_pool,
                        core::BufferPool<audio::sample_t>& sample_buffer_pool,
                        core::IAllocator& allocator);

    //! Add endpoint.
    packet::IWriter* add_endpoint(address::EndpointType type,
                                  address::EndpointProtocol proto);

    //! Update packet queues and sessions.
    void update(packet::timestamp_t timestamp);

    //! Get number of alive sessions.
    size_t num_sessions() const;

private:
    friend class core::RefCnt<ReceiverEndpointSet>;

    void destroy();

    ReceiverEndpoint* create_source_endpoint_(address::EndpointProtocol proto);
    ReceiverEndpoint* create_repair_endpoint_(address::EndpointProtocol proto);

    core::IAllocator& allocator_;

    const rtp::FormatMap& format_map_;

    ReceiverState& receiver_state_;
    ReceiverSessionGroup session_group_;

    core::ScopedPtr<ReceiverEndpoint> source_endpoint_;
    core::ScopedPtr<ReceiverEndpoint> repair_endpoint_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_ENDPOINT_SET_H_
