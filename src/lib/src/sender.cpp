/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/sender.h"

#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/parse_address.h"
#include "roc_pipeline/sender.h"

using namespace roc;

namespace {

// TODO: make this configurable
enum { MaxPacketSize = 2048, MaxFrameSize = 65 * 1024 };

bool make_sender_config(pipeline::SenderConfig& out, const roc_sender_config* in) {
    if (in->samples_per_packet) {
        out.samples_per_packet = in->samples_per_packet;
    }

    switch ((unsigned)in->fec_scheme) {
    case ROC_FEC_RS8M:
        out.fec.codec = fec::ReedSolomon8m;
        break;
    case ROC_FEC_LDPC_STAIRCASE:
        out.fec.codec = fec::LDPCStaircase;
        break;
    case ROC_FEC_NONE:
        out.fec.codec = fec::NoCodec;
        break;
    default:
        return false;
    }

    if (in->n_source_packets || in->n_repair_packets) {
        out.fec.n_source_packets = in->n_source_packets;
        out.fec.n_repair_packets = in->n_repair_packets;
    }

    out.interleaving = !(in->flags & ROC_FLAG_DISABLE_INTERLEAVER);
    out.timing = (in->flags & ROC_FLAG_ENABLE_TIMER);

    return true;
}

bool make_port_config(pipeline::PortConfig& out,
                      roc_protocol proto,
                      const struct sockaddr* addr) {
    switch ((unsigned)proto) {
    case ROC_PROTO_RTP:
        out.protocol = pipeline::Proto_RTP;
        break;
    case ROC_PROTO_RTP_RSM8_SOURCE:
        out.protocol = pipeline::Proto_RTP_RSm8_Source;
        break;
    case ROC_PROTO_RSM8_REPAIR:
        out.protocol = pipeline::Proto_RSm8_Repair;
        break;
    case ROC_PROTO_RTP_LDPC_SOURCE:
        out.protocol = pipeline::Proto_RTP_LDPC_Source;
        break;
    case ROC_PROTO_LDPC_REPAIR:
        out.protocol = pipeline::Proto_LDPC_Repair;
        break;
    default:
        return false;
    }

    if (!out.address.set_saddr(addr)) {
        return false;
    }

    return true;
}

} // namespace

struct roc_sender {
    core::HeapAllocator allocator;

    packet::PacketPool packet_pool;
    core::BufferPool<uint8_t> byte_buffer_pool;
    core::BufferPool<audio::sample_t> sample_buffer_pool;

    rtp::FormatMap format_map;

    pipeline::SenderConfig config;

    netio::Transceiver trx;
    core::UniquePtr<pipeline::Sender> sender;

    packet::IWriter* udp_sender;

    roc_sender(pipeline::SenderConfig& cfg)
        : packet_pool(allocator, 1)
        , byte_buffer_pool(allocator, MaxPacketSize, 1)
        , sample_buffer_pool(allocator, MaxFrameSize, 1)
        , config(cfg)
        , trx(packet_pool, byte_buffer_pool, allocator)
        , udp_sender(NULL) {
    }
};

roc_sender* roc_sender_new(const roc_sender_config* config) {
    pipeline::SenderConfig c;

    if (!make_sender_config(c, config)) {
        return NULL;
    }

    roc_log(LogInfo, "roc sender: creating sender");
    return new roc_sender(c);
}

int roc_sender_bind(roc_sender* sender, struct sockaddr* src_addr) {
    roc_panic_if(!sender);
    roc_panic_if(!src_addr);
    roc_panic_if(sender->udp_sender);

    packet::Address addr;
    if (!addr.set_saddr(src_addr)) {
        return -1;
    }

    sender->udp_sender = sender->trx.add_udp_sender(addr);
    if (!sender->udp_sender) {
        return -1;
    }

    memcpy(src_addr, addr.saddr(), addr.slen());
    return 0;
}

int roc_sender_connect(roc_sender* sender,
                       roc_protocol proto,
                       const struct sockaddr* dst_addr) {
    roc_panic_if(!sender);
    roc_panic_if(!dst_addr);

    pipeline::PortConfig port;
    if (!make_port_config(port, proto, dst_addr)) {
        return -1;
    }

    switch ((unsigned)port.protocol) {
    case pipeline::Proto_RTP:
    case pipeline::Proto_RTP_RSm8_Source:
    case pipeline::Proto_RTP_LDPC_Source:
        sender->config.source_port = port;
        break;

    case pipeline::Proto_RSm8_Repair:
    case pipeline::Proto_LDPC_Repair:
        sender->config.repair_port = port;
        break;

    default:
        return -1;
    }

    return 0;
}

int roc_sender_start(roc_sender* sender) {
    roc_panic_if(!sender);
    roc_panic_if(sender->sender);

    sender->sender.reset(new (sender->allocator) pipeline::Sender(
                             sender->config, *sender->udp_sender, *sender->udp_sender,
                             sender->format_map, sender->packet_pool,
                             sender->byte_buffer_pool, sender->allocator),
                         sender->allocator);

    sender->trx.start();
    return 0;
}

ssize_t
roc_sender_write(roc_sender* sender, const float* samples, const size_t n_samples) {
    roc_panic_if(!sender);
    roc_panic_if(!sender->sender);
    roc_panic_if(!samples && n_samples != 0);

    core::Slice<audio::sample_t> buf(
        new (sender->sample_buffer_pool)
            core::Buffer<audio::sample_t>(sender->sample_buffer_pool));

    buf.resize(n_samples);

    roc_panic_if(sizeof(float) != sizeof(audio::sample_t));
    memcpy(buf.data(), samples, n_samples * sizeof(audio::sample_t));

    audio::Frame frame(buf);
    sender->sender->write(frame);

    return (ssize_t)n_samples;
}

void roc_sender_stop(roc_sender* sender) {
    roc_panic_if(!sender);
    roc_panic_if(!sender->sender);

    sender->trx.stop();
    sender->trx.join();
}

void roc_sender_delete(roc_sender* sender) {
    roc_panic_if(!sender);

    roc_log(LogInfo, "roc sender: deleting sender");
    delete sender;
}
