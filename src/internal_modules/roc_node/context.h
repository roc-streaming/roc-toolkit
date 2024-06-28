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

#include "roc_audio/processor_map.h"
#include "roc_audio/sample.h"
#include "roc_core/allocation_policy.h"
#include "roc_core/atomic.h"
#include "roc_core/iarena.h"
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

    ContextConfig()
        : max_packet_size(2048)
        , max_frame_size(4096) {
    }
};

//! Node context.
class Context : public core::RefCounted<Context, core::NoopAllocation> {
public:
    //! Initialize.
    Context(const ContextConfig& config, core::IArena& arena);

    //! Deinitialize.
    ~Context();

    //! Check if context was successfully constructed.
    status::StatusCode init_status() const;

    //! Get arena.
    core::IArena& arena();

    //! Get packet pool.
    core::IPool& packet_pool();

    //! Get packet buffer pool.
    core::IPool& packet_buffer_pool();

    //! Get frame pool.
    core::IPool& frame_pool();

    //! Get frame buffer pool.
    core::IPool& frame_buffer_pool();

    //! Get processor map.
    audio::ProcessorMap& processor_map();

    //! Get encoding map.
    rtp::EncodingMap& encoding_map();

    //! Get network event loop.
    netio::NetworkLoop& network_loop();

    //! Get control event loop.
    ctl::ControlLoop& control_loop();

private:
    core::IArena& arena_;

    core::SlabPool<packet::Packet> packet_pool_;
    core::SlabPool<core::Buffer> packet_buffer_pool_;

    core::SlabPool<audio::Frame> frame_pool_;
    core::SlabPool<core::Buffer> frame_buffer_pool_;

    audio::ProcessorMap processor_map_;
    rtp::EncodingMap encoding_map_;

    netio::NetworkLoop network_loop_;
    ctl::ControlLoop control_loop_;

    status::StatusCode init_status_;
};

} // namespace node
} // namespace roc

#endif // ROC_NODE_CONTEXT_H_
