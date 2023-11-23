/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/sender_slot.h
//! @brief Sender slot.

#ifndef ROC_PIPELINE_SENDER_SLOT_H_
#define ROC_PIPELINE_SENDER_SLOT_H_

#include "roc_address/interface.h"
#include "roc_address/protocol.h"
#include "roc_audio/fanout.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/optional.h"
#include "roc_core/ref_counted.h"
#include "roc_packet/packet_factory.h"
#include "roc_pipeline/config.h"
#include "roc_pipeline/metrics.h"
#include "roc_pipeline/sender_endpoint.h"
#include "roc_pipeline/sender_session.h"

namespace roc {
namespace pipeline {

//! Sender slot.
//!
//! Contains:
//!  - one or more related sender endpoints, one per each type
//!  - one session associated with those endpoints
class SenderSlot : public core::RefCounted<SenderSlot, core::ArenaAllocation>,
                   public core::ListNode {
public:
    //! Initialize.
    SenderSlot(const SenderConfig& config,
               const rtp::EncodingMap& encoding_map,
               audio::Fanout& fanout,
               packet::PacketFactory& packet_factory,
               core::BufferFactory<uint8_t>& byte_buffer_factory,
               core::BufferFactory<audio::sample_t>& sample_buffer_factory,
               core::IArena& arena);

    ~SenderSlot();

    //! Add endpoint.
    SenderEndpoint* add_endpoint(address::Interface iface,
                                 address::Protocol proto,
                                 const address::SocketAddr& dest_address,
                                 packet::IWriter& dest_writer);

    //! Get audio writer.
    //! @returns NULL if slot is not ready.
    audio::IFrameWriter* writer();

    //! Check if slot configuration is complete.
    bool is_complete() const;

    //! Refresh pipeline according to current time.
    //! @returns
    //!  deadline (absolute time) when refresh should be invoked again
    //!  if there are no frames
    core::nanoseconds_t refresh(core::nanoseconds_t current_time);

    //! Get metrics for slot and its session.
    void get_metrics(SenderSlotMetrics& slot_metrics,
                     SenderSessionMetrics* sess_metrics) const;

private:
    SenderEndpoint* create_source_endpoint_(address::Protocol proto,
                                            const address::SocketAddr& dest_address,
                                            packet::IWriter& dest_writer);
    SenderEndpoint* create_repair_endpoint_(address::Protocol proto,
                                            const address::SocketAddr& dest_address,
                                            packet::IWriter& dest_writer);
    SenderEndpoint* create_control_endpoint_(address::Protocol proto,
                                             const address::SocketAddr& dest_address,
                                             packet::IWriter& dest_writer);

    const SenderConfig& config_;

    audio::Fanout& fanout_;

    core::Optional<SenderEndpoint> source_endpoint_;
    core::Optional<SenderEndpoint> repair_endpoint_;
    core::Optional<SenderEndpoint> control_endpoint_;

    SenderSession session_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_SENDER_SLOT_H_
