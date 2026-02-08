/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_node/context.h
//! @brief Node context.

#ifndef ROC_NODE_CONTEXT_H_
#define ROC_NODE_CONTEXT_H_

#include "roc_audio/sample.h"
#include "roc_core/allocation_policy.h"
#include "roc_core/atomic.h"
#include "roc_core/iarena.h"
#include "roc_core/limited_pool.h"
#include "roc_core/memory_limiter.h"
#include "roc_core/ref_counted.h"
#include "roc_core/slab_pool.h"
#include "roc_ctl/control_loop.h"
#include "roc_netio/network_loop.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace node {

//! Node context config.
struct ContextConfig {
    //! Maximum size in bytes of a network packet.
    size_t max_packet_size;

    //! Maximum size in bytes of an audio frame.
    size_t max_frame_size;

    //! Maximum total bytes for packet pools (packet objects + packet buffers).
    //! 0 = unlimited (no memory limit enforced).
    size_t max_packet_pool_bytes;

    //! Maximum total bytes for frame buffer pool.
    //! 0 = unlimited (no memory limit enforced).
    size_t max_frame_pool_bytes;

    ContextConfig()
        : max_packet_size(2048)
        , max_frame_size(4096)
        , max_packet_pool_bytes(32 * 1024 * 1024)
        , max_frame_pool_bytes(8 * 1024 * 1024) {
    }
};

//! Node context.
class Context : public core::RefCounted<Context, core::ManualAllocation> {
public:
    //! Initialize.
    explicit Context(const ContextConfig& config, core::IArena& arena);

    //! Deinitialize.
    ~Context();

    //! Check if successfully constructed.
    bool is_valid();

    //! Get arena.
    core::IArena& arena();

    //! Get packet pool.
    core::IPool& packet_pool();

    //! Get packet buffer pool.
    core::IPool& packet_buffer_pool();

    //! Get frame buffer pool.
    core::IPool& frame_buffer_pool();

    //! Get encoding map.
    rtp::EncodingMap& encoding_map();

    //! Get network event loop.
    netio::NetworkLoop& network_loop();

    //! Get control event loop.
    ctl::ControlLoop& control_loop();

private:
    core::IArena& arena_;

    // Memory limiters (destroyed last, after all pools release memory).
    core::MemoryLimiter packet_mem_limiter_;
    core::MemoryLimiter frame_mem_limiter_;

    // Underlying slab pools.
    core::SlabPool<packet::Packet> packet_pool_;
    core::SlabPool<core::Buffer> packet_buffer_pool_;
    core::SlabPool<core::Buffer> frame_buffer_pool_;

    // Limited pool wrappers (destroyed before slab pools).
    core::LimitedPool limited_packet_pool_;
    core::LimitedPool limited_packet_buffer_pool_;
    core::LimitedPool limited_frame_buffer_pool_;

    rtp::EncodingMap encoding_map_;

    // Event loops (destroyed first, returning all objects to pools).
    netio::NetworkLoop network_loop_;
    ctl::ControlLoop control_loop_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_CONTEXT_H_
