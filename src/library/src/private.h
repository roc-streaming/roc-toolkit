/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PRIVATE_H_
#define ROC_PRIVATE_H_

#include "roc/address.h"
#include "roc/context.h"
#include "roc/receiver.h"
#include "roc/sender.h"

#include "roc_address/socket_addr.h"
#include "roc_audio/units.h"
#include "roc_core/array.h"
#include "roc_core/atomic.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/mutex.h"
#include "roc_core/scoped_ptr.h"
#include "roc_netio/event_loop.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet_pool.h"
#include "roc_pipeline/receiver_source.h"
#include "roc_pipeline/sender_sink.h"
#include "roc_rtp/format_map.h"

const roc::address::SocketAddr& get_address(const roc_address* address);
roc::address::SocketAddr& get_address(roc_address* address);

bool make_context_config(roc_context_config& out, const roc_context_config& in);

bool make_sender_config(roc::pipeline::SenderConfig& out, const roc_sender_config& in);
bool make_receiver_config(roc::pipeline::ReceiverConfig& out,
                          const roc_receiver_config& in);

bool make_port_config(roc::pipeline::PortConfig& out,
                      roc_port_type type,
                      roc_protocol proto,
                      const roc::address::SocketAddr& addr);

struct roc_context {
    roc_context(const roc_context_config& cfg);

    roc::core::HeapAllocator allocator;

    roc::packet::PacketPool packet_pool;
    roc::core::BufferPool<uint8_t> byte_buffer_pool;
    roc::core::BufferPool<roc::audio::sample_t> sample_buffer_pool;

    roc::netio::EventLoop event_loop;

    roc::core::Atomic counter;
};

struct roc_sender {
    roc_sender(roc_context& ctx, roc::pipeline::SenderConfig& cfg);

    roc_context& context;

    roc::fec::CodecMap codec_map;
    roc::rtp::FormatMap format_map;

    roc::pipeline::SenderConfig config;

    roc::pipeline::PortConfig source_port;
    roc::pipeline::PortConfig repair_port;

    roc::core::ScopedPtr<roc::pipeline::SenderSink> sender;
    roc::packet::IWriter* writer;

    roc::address::SocketAddr address;

    roc::core::Mutex mutex;

    size_t num_channels;
};

struct roc_receiver {
    roc_receiver(roc_context& ctx, roc::pipeline::ReceiverConfig& cfg);

    roc_context& context;

    roc::fec::CodecMap codec_map;
    roc::rtp::FormatMap format_map;

    roc::pipeline::ReceiverSource receiver;

    size_t num_channels;

    roc::core::Array<roc::address::SocketAddr, 2> addresses;
};

#endif // ROC_PRIVATE_H_
