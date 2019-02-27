/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "private.h"

#include "roc_audio/resampler_profile.h"
#include "roc_core/log.h"

using namespace roc;

bool config_context(roc_context_config& out, const roc_context_config* in) {
    out.max_packet_size = 2048;
    out.max_frame_size = 4096;

    if (in) {
        if (in->max_packet_size != 0) {
            out.max_packet_size = in->max_packet_size;
        }
        if (in->max_frame_size / sizeof(audio::sample_t) != 0) {
            out.max_frame_size = in->max_frame_size;
        }
    }

    return true;
}

bool config_sender(pipeline::SenderConfig& out, const roc_sender_config& in) {
    if (in.packet_length != 0) {
        out.packet_length = (core::nanoseconds_t)in.packet_length;
    }

    if (in.input_sample_rate != 0) {
        out.input_sample_rate = in.input_sample_rate;
    }

    switch ((unsigned)in.fec_scheme) {
    case ROC_FEC_DISABLE:
        out.fec.codec = fec::NoCodec;
        break;
    case ROC_FEC_DEFAULT:
    case ROC_FEC_RS8M:
        out.fec.codec = fec::ReedSolomon8m;
        break;
    case ROC_FEC_LDPC_STAIRCASE:
        out.fec.codec = fec::LDPCStaircase;
        break;
    default:
        roc_log(LogError, "roc_config: invalid fec_scheme");
        return false;
    }

    if (in.fec_block_source_packets || in.fec_block_repair_packets) {
        out.fec.n_source_packets = in.fec_block_source_packets;
        out.fec.n_repair_packets = in.fec_block_repair_packets;
    }

    switch ((unsigned)in.resampler_profile) {
    case ROC_RESAMPLER_DISABLE:
        break;
    case ROC_RESAMPLER_LOW:
        out.resampler = audio::resampler_profile(audio::ResamplerProfile_Low);
        break;
    case ROC_RESAMPLER_DEFAULT:
    case ROC_RESAMPLER_MEDIUM:
        out.resampler = audio::resampler_profile(audio::ResamplerProfile_Medium);
        break;
    case ROC_RESAMPLER_HIGH:
        out.resampler = audio::resampler_profile(audio::ResamplerProfile_High);
        break;
    default:
        roc_log(LogError, "roc_config: invalid resampler_profile");
        return false;
    }

    out.resampling = (in.resampler_profile != ROC_RESAMPLER_DISABLE);
    out.interleaving = in.packet_interleaving;
    out.timing = in.automatic_timing;

    return true;
}

bool config_receiver(pipeline::ReceiverConfig& out, const roc_receiver_config& in) {
    if (in.target_latency != 0) {
        out.default_session.target_latency = (core::nanoseconds_t)in.target_latency;

        out.default_session.latency_monitor.min_latency =
            (core::nanoseconds_t)in.target_latency * pipeline::DefaultMinLatencyFactor;

        out.default_session.latency_monitor.max_latency =
            (core::nanoseconds_t)in.target_latency * pipeline::DefaultMaxLatencyFactor;
    }

    if (in.min_latency != 0) {
        out.default_session.latency_monitor.min_latency = in.min_latency;
    }

    if (in.max_latency != 0) {
        out.default_session.latency_monitor.max_latency =
            (core::nanoseconds_t)in.max_latency;
    }

    if (in.no_playback_timeout < 0) {
        out.default_session.watchdog.no_playback_timeout = 0;
    } else if (in.no_playback_timeout > 0) {
        out.default_session.watchdog.no_playback_timeout = in.no_playback_timeout;
    }

    if (in.broken_playback_timeout < 0) {
        out.default_session.watchdog.broken_playback_timeout = 0;
    } else if (in.no_playback_timeout > 0) {
        out.default_session.watchdog.broken_playback_timeout = in.broken_playback_timeout;
    }

    if (in.breakage_detection_window != 0) {
        out.default_session.watchdog.breakage_detection_window =
            (core::nanoseconds_t)in.breakage_detection_window;
    }

    if (in.packet_length != 0) {
        out.default_session.packet_length = (core::nanoseconds_t)in.packet_length;
    }

    if (in.output_sample_rate != 0) {
        out.output.sample_rate = in.output_sample_rate;
    }

    switch ((unsigned)in.fec_scheme) {
    case ROC_FEC_DISABLE:
        out.default_session.fec.codec = fec::NoCodec;
        break;
    case ROC_FEC_DEFAULT:
    case ROC_FEC_RS8M:
        out.default_session.fec.codec = fec::ReedSolomon8m;
        break;
    case ROC_FEC_LDPC_STAIRCASE:
        out.default_session.fec.codec = fec::LDPCStaircase;
        break;
    default:
        roc_log(LogError, "roc_config: invalid fec_scheme");
        return false;
    }

    if (in.fec_block_source_packets || in.fec_block_repair_packets) {
        out.default_session.fec.n_source_packets = in.fec_block_source_packets;
        out.default_session.fec.n_repair_packets = in.fec_block_repair_packets;
    }

    switch ((unsigned)in.resampler_profile) {
    case ROC_RESAMPLER_DISABLE:
        break;
    case ROC_RESAMPLER_LOW:
        out.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_Low);
        break;
    case ROC_RESAMPLER_DEFAULT:
    case ROC_RESAMPLER_MEDIUM:
        out.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_Medium);
        break;
    case ROC_RESAMPLER_HIGH:
        out.default_session.resampler =
            audio::resampler_profile(audio::ResamplerProfile_High);
        break;
    default:
        roc_log(LogError, "roc_config: invalid resampler_profile");
        return false;
    }

    out.output.resampling = (in.resampler_profile != ROC_RESAMPLER_DISABLE);
    out.output.timing = in.automatic_timing;

    return true;
}

bool config_port(pipeline::PortConfig& out,
                 roc_protocol proto,
                 const packet::Address& addr) {
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

    out.address = addr;

    return true;
}
