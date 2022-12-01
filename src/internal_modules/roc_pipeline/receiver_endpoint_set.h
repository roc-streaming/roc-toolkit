/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_endpoint_set.h
//! @brief Receiver endpoint set.

#ifndef ROC_PIPELINE_RECEIVER_ENDPOINT_SET_H_
#define ROC_PIPELINE_RECEIVER_ENDPOINT_SET_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_audio/mixer.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/list_node.h"
#include "roc_core/ref_counted.h"
#include "roc_packet/packet_factory.h"
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
class ReceiverEndpointSet
    : public core::RefCounted<ReceiverEndpointSet, core::StandardAllocation>,
      public core::ListNode {
    typedef core::RefCounted<ReceiverEndpointSet, core::StandardAllocation> RefCounted;

public:
    //! Initialize.
    ReceiverEndpointSet(const ReceiverConfig& receiver_config,
                        ReceiverState& receiver_state,
                        audio::Mixer& mixer,
                        const rtp::FormatMap& format_map,
                        packet::PacketFactory& packet_factory,
                        core::BufferFactory<uint8_t>& byte_buffer_factory,
                        core::BufferFactory<audio::sample_t>& sample_buffer_factory,
                        core::IAllocator& allocator);

    //! Create endpoint.
    ReceiverEndpoint* create_endpoint(address::Interface iface, address::Protocol proto);

    //! Delete endpoint.
    void delete_endpoint(address::Interface iface);

    //! Pull packets from queues and advance session timestamp.
    void advance(packet::timestamp_t timestamp);

    //! Adjust session clock to match consumer clock.
    void reclock(packet::ntp_timestamp_t timestamp);

    //! Get number of alive sessions.
    size_t num_sessions() const;

private:
    ReceiverEndpoint* create_source_endpoint_(address::Protocol proto);
    ReceiverEndpoint* create_repair_endpoint_(address::Protocol proto);
    ReceiverEndpoint* create_control_endpoint_(address::Protocol proto);

    const rtp::FormatMap& format_map_;

    ReceiverState& receiver_state_;
    ReceiverSessionGroup session_group_;

    core::Optional<ReceiverEndpoint> source_endpoint_;
    core::Optional<ReceiverEndpoint> repair_endpoint_;
    core::Optional<ReceiverEndpoint> control_endpoint_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_ENDPOINT_SET_H_
