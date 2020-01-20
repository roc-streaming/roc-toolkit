/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_peer/context.h
//! @brief Peer context.

#ifndef ROC_PEER_CONTEXT_H_
#define ROC_PEER_CONTEXT_H_

#include "roc_audio/units.h"
#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_netio/event_loop.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace peer {

//! Peer context config.
struct ContextConfig {
    //! Maximum size in bytes of a network packet.
    size_t max_packet_size;

    //! Maximum size in bytes of an audio frame.
    size_t max_frame_size;

    //! Enable memory poisoning.
    bool poisoning;

    ContextConfig()
        : max_packet_size(2048)
        , max_frame_size(4096)
        , poisoning(false) {
    }
};

//! Peer context.
class Context : public core::NonCopyable<> {
public:
    //! Initialize.
    explicit Context(const ContextConfig& config);

    //! Deinitialize.
    ~Context();

    //! Check if successfully constructed.
    bool valid();

    //! Increment context reference counter.
    void incref();

    //! Decrement context reference counter.
    void decref();

    //! Check if context is still in use.
    bool is_used();

    //! Get allocator.
    core::IAllocator& allocator();

    //! Get packet pool.
    packet::PacketPool& packet_pool();

    //! Byte byte buffer pool.
    core::BufferPool<uint8_t>& byte_buffer_pool();

    //! Byte sample buffer pool.
    core::BufferPool<audio::sample_t>& sample_buffer_pool();

    //! Get network event loop.
    netio::EventLoop& event_loop();

private:
    core::HeapAllocator allocator_;

    packet::PacketPool packet_pool_;
    core::BufferPool<uint8_t> byte_buffer_pool_;
    core::BufferPool<audio::sample_t> sample_buffer_pool_;

    netio::EventLoop event_loop_;

    core::Atomic ref_counter_;
};

} // namespace peer
} // namespace roc

#endif // ROC_PEER_CONTEXT_H_
