/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "private.h"

#include "roc_core/log.h"
#include "roc_core/panic.h"

using namespace roc;

namespace {

bool make_receiver_config(pipeline::ReceiverConfig& out, const roc_receiver_config* in) {
    if (in->latency) {
        out.default_session.latency = in->latency;

        out.default_session.latency_monitor.min_latency =
            (packet::signed_timestamp_t)in->latency * pipeline::DefaultMinLatency;

        out.default_session.latency_monitor.max_latency =
            (packet::signed_timestamp_t)in->latency * pipeline::DefaultMaxLatency;
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

void close_port(void* arg, const pipeline::PortConfig& port) {
    roc_panic_if_not(arg);
    roc_receiver* receiver = (roc_receiver*)arg;

    receiver->context.trx.remove_port(port.address);
}

} // namespace

roc_receiver::roc_receiver(roc_context& ctx, pipeline::ReceiverConfig& cfg)
    : context(ctx)
    , receiver(cfg,
               format_map,
               context.packet_pool,
               context.byte_buffer_pool,
               context.sample_buffer_pool,
               context.allocator) {
}

roc_receiver* roc_receiver_open(roc_context* context, const roc_receiver_config* config) {
    roc_panic_if_not(context);

    pipeline::ReceiverConfig c;
    if (config) {
        if (!make_receiver_config(c, config)) {
            return NULL;
        }
    }

    roc_log(LogInfo, "roc receiver: opening receiver");

    roc_receiver* r = new(std::nothrow) roc_receiver(*context, c);
    if (!r) {
        return NULL;
    }

    ++context->refcount;
    return r;
}

int roc_receiver_bind(roc_receiver* receiver, roc_protocol proto, struct sockaddr* addr) {
    roc_panic_if(!receiver);
    roc_panic_if(!addr);

    pipeline::PortConfig port;
    if (!make_port_config(port, proto, addr)) {
        return -1;
    }

    if (!receiver->context.trx.add_udp_receiver(port.address, receiver->receiver)) {
        return -1;
    }

    if (!receiver->receiver.add_port(port)) {
        return -1;
    }

    memcpy(addr, port.address.saddr(), port.address.slen());
    return 0;
}

ssize_t
roc_receiver_read(roc_receiver* receiver, float* samples, const size_t n_samples) {
    roc_panic_if(!receiver);
    roc_panic_if(!samples && n_samples != 0);

    core::Slice<audio::sample_t> buf(
        new (receiver->context.sample_buffer_pool)
            core::Buffer<audio::sample_t>(receiver->context.sample_buffer_pool));
    buf.resize(n_samples);

    audio::Frame frame(buf);
    receiver->receiver.read(frame);

    roc_panic_if(sizeof(float) != sizeof(audio::sample_t));
    memcpy(samples, frame.samples().data(), n_samples * sizeof(audio::sample_t));

    return (ssize_t)n_samples;
}

void roc_receiver_close(roc_receiver* receiver) {
    roc_panic_if(!receiver);

    roc_log(LogInfo, "roc receiver: closing receiver");

    receiver->receiver.iterate_ports(close_port, receiver);

    --receiver->context.refcount;

    delete receiver;
}
