/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/receiver.h"

#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/parse_address.h"
#include "roc_pipeline/receiver.h"

using namespace roc;

namespace {

// TODO: make this configurable
enum { MaxPacketSize = 2048, MaxFrameSize = 65 * 1024 };

bool make_receiver_config(pipeline::ReceiverConfig& out, const roc_receiver_config* in) {
    if (in->latency) {
        out.default_session.latency = in->latency;
    }

    if (in->timeout) {
        out.default_session.timeout = in->timeout;
    }

    if (in->samples_per_packet) {
        out.default_session.samples_per_packet = in->samples_per_packet;
    }

    if (in->sample_rate) {
        out.sample_rate = in->sample_rate;
    }

    switch ((unsigned)in->fec_scheme) {
    case ROC_FEC_RS8M:
        out.default_session.fec.codec = fec::ReedSolomon8m;
        break;
    case ROC_FEC_LDPC_STAIRCASE:
        out.default_session.fec.codec = fec::LDPCStaircase;
        break;
    case ROC_FEC_NONE:
        out.default_session.fec.codec = fec::NoCodec;
        break;
    default:
        return false;
    }

    if (in->n_source_packets || in->n_repair_packets) {
        out.default_session.fec.n_source_packets = in->n_source_packets;
        out.default_session.fec.n_repair_packets = in->n_repair_packets;
    }

    out.default_session.resampling = !(in->flags & ROC_FLAG_DISABLE_RESAMPLER);
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

struct roc_receiver {
    core::HeapAllocator allocator;

    packet::PacketPool packet_pool;
    core::BufferPool<uint8_t> byte_buffer_pool;
    core::BufferPool<audio::sample_t> sample_buffer_pool;

    rtp::FormatMap format_map;

    pipeline::Receiver receiver;
    netio::Transceiver trx;

    roc_receiver(pipeline::ReceiverConfig& config)
        : packet_pool(allocator, 1)
        , byte_buffer_pool(allocator, MaxPacketSize, 1)
        , sample_buffer_pool(allocator, MaxFrameSize, 1)
        , receiver(config,
                   format_map,
                   packet_pool,
                   byte_buffer_pool,
                   sample_buffer_pool,
                   allocator)
        , trx(packet_pool, byte_buffer_pool, allocator) {
    }
};

roc_receiver* roc_receiver_new(const roc_receiver_config* config) {
    pipeline::ReceiverConfig c;

    if (!make_receiver_config(c, config)) {
        return NULL;
    }

    roc_log(LogInfo, "roc receiver: creating receiver");
    return new roc_receiver(c);
}

int roc_receiver_bind(roc_receiver* receiver, roc_protocol proto, struct sockaddr* addr) {
    roc_panic_if(!receiver);
    roc_panic_if(!addr);

    pipeline::PortConfig port;
    if (!make_port_config(port, proto, addr)) {
        return -1;
    }

    if (!receiver->trx.add_udp_receiver(port.address, receiver->receiver)) {
        return -1;
    }

    if (!receiver->receiver.add_port(port)) {
        return -1;
    }

    memcpy(addr, port.address.saddr(), port.address.slen());
    return 0;
}

int roc_receiver_start(roc_receiver* receiver) {
    roc_panic_if(receiver == NULL);

    receiver->trx.start();
    return 0;
}

ssize_t
roc_receiver_read(roc_receiver* receiver, float* samples, const size_t n_samples) {
    roc_panic_if(!receiver);
    roc_panic_if(!samples && n_samples != 0);

    core::Slice<audio::sample_t> buf(
        new (receiver->sample_buffer_pool)
            core::Buffer<audio::sample_t>(receiver->sample_buffer_pool));
    buf.resize(n_samples);

    audio::Frame frame(buf);
    receiver->receiver.read(frame);

    roc_panic_if(sizeof(float) != sizeof(audio::sample_t));
    memcpy(samples, frame.samples().data(), n_samples * sizeof(audio::sample_t));

    return (ssize_t)n_samples;
}

void roc_receiver_stop(roc_receiver* receiver) {
    roc_panic_if(!receiver);

    receiver->trx.stop();
    receiver->trx.join();
}

void roc_receiver_delete(roc_receiver* receiver) {
    roc_panic_if(!receiver);

    roc_log(LogInfo, "roc receiver: deleting receiver");
    delete receiver;
}
