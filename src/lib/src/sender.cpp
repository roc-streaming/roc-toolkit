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

roc_sender::roc_sender(roc_context& ctx, pipeline::SenderConfig& cfg)
    : context(ctx)
    , config(cfg)
    , writer(NULL) {
}

roc_sender* roc_sender_open(roc_context* context, const roc_sender_config* config) {
    roc_panic_if_not(context);

    pipeline::SenderConfig c;
    if (config) {
        if (!make_sender_config(c, config)) {
            return NULL;
        }
    }

    roc_log(LogInfo, "roc sender: opening sender");

    roc_sender* s = new(std::nothrow) roc_sender(*context, c);
    if (!s) {
        return NULL;
    }

    ++context->refcount;
    return s;
}

int roc_sender_bind(roc_sender* sender, struct sockaddr* src_addr) {
    roc_panic_if(!sender);
    roc_panic_if(!src_addr);
    roc_panic_if(sender->writer);

    if (!sender->address.set_saddr(src_addr)) {
        return -1;
    }

    sender->writer = sender->context.trx.add_udp_sender(sender->address);
    if (!sender->writer) {
        return -1;
    }

    memcpy(src_addr, sender->address.saddr(), sender->address.slen());
    return 0;
}

int roc_sender_connect(roc_sender* sender,
                       roc_protocol proto,
                       const struct sockaddr* dst_addr) {
    roc_panic_if(!sender);
    roc_panic_if(!dst_addr);
    roc_panic_if(sender->sender);

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

ssize_t
roc_sender_write(roc_sender* sender, const float* samples, const size_t n_samples) {
    roc_panic_if(!sender);
    roc_panic_if(!samples && n_samples != 0);

    if (!sender->sender) {
        sender->sender.reset(new (sender->context.allocator) pipeline::Sender(
                                 sender->config, *sender->writer, *sender->writer,
                                 sender->format_map, sender->context.packet_pool,
                                 sender->context.byte_buffer_pool,
                                 sender->context.allocator),
                             sender->context.allocator);

        if (!sender->sender) {
            return -1;
        }
    }

    if (!sender->sender->valid()) {
        return -1;
    }

    core::Slice<audio::sample_t> buf(
        new (sender->context.sample_buffer_pool)
            core::Buffer<audio::sample_t>(sender->context.sample_buffer_pool));

    buf.resize(n_samples);

    roc_panic_if(sizeof(float) != sizeof(audio::sample_t));
    memcpy(buf.data(), samples, n_samples * sizeof(audio::sample_t));

    audio::Frame frame(buf);
    sender->sender->write(frame);

    return (ssize_t)n_samples;
}

void roc_sender_close(roc_sender* sender) {
    roc_panic_if(!sender);

    roc_log(LogInfo, "roc sender: closing sender");

    if (sender->writer) {
        sender->context.trx.remove_port(sender->address);
    }

    --sender->context.refcount;

    delete sender;
}
