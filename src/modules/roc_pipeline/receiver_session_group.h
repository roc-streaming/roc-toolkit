/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_pipeline/receiver_session_group.h
//! @brief Receiver session group.

#ifndef ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_
#define ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_

#include "roc_audio/mixer.h"
#include "roc_core/iallocator.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_pipeline/receiver_session.h"
#include "roc_pipeline/receiver_state.h"

namespace roc {
namespace pipeline {

//! Receiver session group.
class ReceiverSessionGroup : public core::NonCopyable<> {
public:
    //! Initialize.
    ReceiverSessionGroup(const ReceiverConfig& receiver_config,
                         ReceiverState& receiver_state,
                         audio::Mixer& mixer,
                         const fec::CodecMap& codec_map,
                         const rtp::FormatMap& format_map,
                         packet::PacketPool& packet_pool,
                         core::BufferPool<uint8_t>& byte_buffer_pool,
                         core::BufferPool<audio::sample_t>& sample_buffer_pool,
                         core::IAllocator& allocator);

    //! Route packet to session.
    void route_packet(const packet::PacketPtr& packet);

    //! Update sessions states.
    void update_sessions(packet::timestamp_t timestamp);

    //! Get number of alive sessions.
    size_t num_sessions() const;

private:
    bool can_create_session_(const packet::PacketPtr& packet);

    void create_session_(const packet::PacketPtr& packet);
    void remove_session_(ReceiverSession& sess);

    ReceiverSessionConfig make_session_config_(const packet::PacketPtr& packet) const;

    core::IAllocator& allocator_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& byte_buffer_pool_;
    core::BufferPool<audio::sample_t>& sample_buffer_pool_;

    const fec::CodecMap& codec_map_;
    const rtp::FormatMap& format_map_;

    audio::Mixer& mixer_;

    ReceiverState& receiver_state_;
    const ReceiverConfig& receiver_config_;

    core::List<ReceiverSession> sessions_;
};

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_RECEIVER_SESSION_GROUP_H_
