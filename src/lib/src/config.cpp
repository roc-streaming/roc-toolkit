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

using namespace roc;

bool config_context(roc_context_config& out, const roc_context_config* in) {
    enum {
        DefaultMaxPacketSize = 2048,
        DefaultMaxFrameSize = 64 * 1024,
        DefaultChunkSize = 128 * 1024
    };

    out.max_packet_size = DefaultMaxPacketSize;
    out.max_frame_size = DefaultMaxFrameSize;
    out.chunk_size = DefaultChunkSize;

    if (in) {
        if (in->max_packet_size) {
            out.max_packet_size = in->max_packet_size;
        }
        if (in->max_frame_size) {
            out.max_frame_size = in->max_frame_size;
        }
        if (in->chunk_size) {
            out.chunk_size = in->chunk_size;
        }
    }

    if (out.chunk_size / out.max_packet_size == 0) {
        roc_log(LogError, "roc_config: invalid config:"
                          " (chunk_size / max_packet_size == 0):"
                          " chunk_size=%lu max_packet_size=%lu",
                (unsigned long)out.chunk_size, (unsigned long)out.max_packet_size);
        return false;
    }

    if (out.chunk_size / out.max_frame_size == 0) {
        roc_log(LogError, "roc_config: invalid config:"
                          " (chunk_size / max_frame_size == 0):"
                          " chunk_size=%lu max_frame_size=%lu",
                (unsigned long)out.chunk_size, (unsigned long)out.max_frame_size);
        return false;
    }

    if (out.max_frame_size / sizeof(audio::sample_t) == 0) {
        roc_log(LogError, "roc_config: invalid config:"
                          " (max_frame_size / sizeof(sample_t) == 0):"
                          " max_frame_size=%lu sizeof(sample_t)=%lu",
                (unsigned long)out.max_frame_size,
                (unsigned long)sizeof(audio::sample_t));
        return false;
    }

    return true;
}

bool config_sender(pipeline::SenderConfig& out, const roc_sender_config& in) {
    if (in.samples_per_packet) {
        out.samples_per_packet = in.samples_per_packet;
    }

    switch ((unsigned)in.fec_scheme) {
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
        roc_log(LogError, "roc_config: invalid fec_scheme");
        return false;
    }

    if (in.n_source_packets || in.n_repair_packets) {
        out.fec.n_source_packets = in.n_source_packets;
        out.fec.n_repair_packets = in.n_repair_packets;
    }

    out.interleaving = !(in.flags & ROC_FLAG_DISABLE_INTERLEAVER);
    out.timing = (in.flags & ROC_FLAG_ENABLE_TIMER);

    return true;
}

bool config_receiver(pipeline::ReceiverConfig& out, const roc_receiver_config& in) {
    if (in.latency) {
        out.default_session.latency = in.latency;

        out.default_session.latency_monitor.min_latency =
            (packet::signed_timestamp_t)in.latency * pipeline::DefaultMinLatency;

        out.default_session.latency_monitor.max_latency =
            (packet::signed_timestamp_t)in.latency * pipeline::DefaultMaxLatency;
    }

    if (in.timeout) {
        out.default_session.timeout = in.timeout;
    }

    if (in.samples_per_packet) {
        out.default_session.samples_per_packet = in.samples_per_packet;
    }

    if (in.sample_rate) {
        out.sample_rate = in.sample_rate;
    }

    switch ((unsigned)in.fec_scheme) {
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
        roc_log(LogError, "roc_config: invalid fec_scheme");
        return false;
    }

    if (in.n_source_packets || in.n_repair_packets) {
        out.default_session.fec.n_source_packets = in.n_source_packets;
        out.default_session.fec.n_repair_packets = in.n_repair_packets;
    }

    out.default_session.resampling = !(in.flags & ROC_FLAG_DISABLE_RESAMPLER);
    out.timing = (in.flags & ROC_FLAG_ENABLE_TIMER);

    return true;
}

bool config_port(pipeline::PortConfig& out,
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
        roc_log(LogError, "roc_config: invalid protocol");
        return false;
    }

    if (!out.address.set_saddr(addr)) {
        roc_log(LogError, "roc_config: invalid address");
        return false;
    }

    return true;
}
