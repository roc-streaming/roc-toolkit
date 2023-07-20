/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "config_helpers.h"

#include "roc_audio/resampler_profile.h"
#include "roc_core/attributes.h"
#include "roc_core/log.h"

namespace roc {
namespace api {

bool context_config_from_user(peer::ContextConfig& out, const roc_context_config& in) {
    if (in.max_packet_size != 0) {
        out.max_packet_size = in.max_packet_size;
    }

    if (in.max_frame_size != 0) {
        out.max_frame_size = in.max_frame_size;
    }

    return true;
}

bool sender_config_from_user(peer::Context& context,
                             pipeline::SenderConfig& out,
                             const roc_sender_config& in) {
    if (in.frame_format != ROC_FORMAT_PCM_FLOAT32) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.frame_format:"
                " should be valid enum value");
        return false;
    }

    if (in.frame_channels != 0) {
        if (!channel_set_from_user(out.input_sample_spec.channel_set(),
                                   in.frame_channels)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_sender_config.frame_channels:"
                    " should be valid enum value");
            return false;
        }
    } else {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.frame_channels:"
                " should be non-zero");
        return false;
    }

    if (in.frame_sample_rate != 0) {
        out.input_sample_spec.set_sample_rate(in.frame_sample_rate);
    } else {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.frame_sample_rate:"
                " should be non-zero");
        return false;
    }

    if (in.packet_encoding != 0) {
        if (!packet_encoding_from_user(out.payload_type, in.packet_encoding)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_sender_config.packet_encoding:"
                    " should be zero or valid enum value");
            return false;
        }
    } else {
        const rtp::Format* format =
            context.format_map().find_by_spec(out.input_sample_spec);
        if (!format) {
            roc_log(LogError,
                    "bad configuration:"
                    " failed to select packet_encoding matching frame_encoding,"
                    " set roc_sender_config.packet_encoding manually");
            return false;
        }
        out.payload_type = (rtp::PayloadType)format->payload_type;
    }

    if (in.packet_length != 0) {
        out.packet_length = (core::nanoseconds_t)in.packet_length;
    }

    out.interleaving = in.packet_interleaving;

    if (!fec_encoding_from_user(out.fec_encoder.scheme, in.fec_encoding)) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.fec_encoding:"
                " should be valid enum value");
        return false;
    }

    if (in.fec_block_source_packets != 0 || in.fec_block_repair_packets != 0) {
        out.fec_writer.n_source_packets = in.fec_block_source_packets;
        out.fec_writer.n_repair_packets = in.fec_block_repair_packets;
    }

    if (!clock_source_from_user(out.timing, in.clock_source)) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.clock_source:"
                " should be valid enum value");
        return false;
    }

    if (!resampler_backend_from_user(out.resampler_backend, in.resampler_backend)) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.resampler_backend:"
                " should be valid enum value");
        return false;
    }

    if (!resampler_profile_from_user(out.resampler_profile, in.resampler_profile)) {
        roc_log(LogError,
                "bad configuration: invalid roc_sender_config.resampler_profile:"
                " should be valid enum value");
        return false;
    }

    out.resampling = (in.resampler_profile != ROC_RESAMPLER_PROFILE_DISABLE);

    return true;
}

bool receiver_config_from_user(peer::Context&,
                               pipeline::ReceiverConfig& out,
                               const roc_receiver_config& in) {
    if (in.frame_format != ROC_FORMAT_PCM_FLOAT32) {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.frame_format:"
                " should be valid enum value");
        return false;
    }

    if (in.frame_channels != 0) {
        if (!channel_set_from_user(out.common.output_sample_spec.channel_set(),
                                   in.frame_channels)) {
            roc_log(LogError,
                    "bad configuration: invalid roc_receiver_config.frame_channels:"
                    " should be valid enum value");
            return false;
        }
    } else {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.frame_channels:"
                " should be non-zero");
        return false;
    }

    if (in.frame_sample_rate != 0) {
        out.common.output_sample_spec.set_sample_rate(in.frame_sample_rate);
    } else {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.frame_sample_rate:"
                " should be non-zero");
        return false;
    }

    if (!clock_source_from_user(out.common.timing, in.clock_source)) {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.clock_source:"
                " should be valid enum value");
        return false;
    }

    if (!resampler_backend_from_user(out.default_session.resampler_backend,
                                     in.resampler_backend)) {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.resampler_backend:"
                " should be valid enum value");
        return false;
    }

    if (!resampler_profile_from_user(out.default_session.resampler_profile,
                                     in.resampler_profile)) {
        roc_log(LogError,
                "bad configuration: invalid roc_receiver_config.resampler_profile:"
                " should be valid enum value");
        return false;
    }

    out.common.resampling = (in.resampler_profile != ROC_RESAMPLER_PROFILE_DISABLE);

    if (in.target_latency != 0) {
        out.default_session.target_latency = (core::nanoseconds_t)in.target_latency;
    }

    if (in.latency_tolerance != 0) {
        out.default_session.latency_monitor.min_latency =
            out.default_session.target_latency
            - (core::nanoseconds_t)in.latency_tolerance;

        out.default_session.latency_monitor.max_latency =
            out.default_session.target_latency
            + (core::nanoseconds_t)in.latency_tolerance;
    } else {
        out.default_session.latency_monitor.deduce_min_latency(
            out.default_session.target_latency);

        out.default_session.latency_monitor.deduce_max_latency(
            out.default_session.target_latency);
    }

    if (in.no_playback_timeout < 0) {
        out.default_session.watchdog.no_playback_timeout = 0;
    } else if (in.no_playback_timeout > 0) {
        out.default_session.watchdog.no_playback_timeout = in.no_playback_timeout;
    }

    if (in.choppy_playback_timeout < 0) {
        out.default_session.watchdog.choppy_playback_timeout = 0;
    } else if (in.choppy_playback_timeout > 0) {
        out.default_session.watchdog.choppy_playback_timeout = in.choppy_playback_timeout;

        out.default_session.watchdog.deduce_choppy_playback_window(
            out.default_session.watchdog.choppy_playback_timeout);
    }

    return true;
}

ROC_ATTR_NO_SANITIZE_UB
bool channel_set_from_user(audio::ChannelSet& out, roc_channel_layout in) {
    switch (in) {
    case ROC_CHANNEL_LAYOUT_MONO:
        out.set_layout(audio::ChannelLayout_Mono);
        out.set_channel_mask(audio::ChannelMask_Mono);
        return true;

    case ROC_CHANNEL_LAYOUT_STEREO:
        out.set_layout(audio::ChannelLayout_Surround);
        out.set_channel_mask(audio::ChannelMask_Stereo);
        return true;

    default:
        break;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool clock_source_from_user(bool& timing, roc_clock_source in) {
    switch (in) {
    case ROC_CLOCK_EXTERNAL:
        timing = false;
        return true;

    case ROC_CLOCK_INTERNAL:
        timing = true;
        return true;

    default:
        break;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool resampler_backend_from_user(audio::ResamplerBackend& out, roc_resampler_backend in) {
    switch (in) {
    case ROC_RESAMPLER_BACKEND_DEFAULT:
        out = audio::ResamplerBackend_Default;
        return true;

    case ROC_RESAMPLER_BACKEND_BUILTIN:
        out = audio::ResamplerBackend_Builtin;
        return true;

    case ROC_RESAMPLER_BACKEND_SPEEX:
        out = audio::ResamplerBackend_Speex;
        return true;

    default:
        break;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool resampler_profile_from_user(audio::ResamplerProfile& out, roc_resampler_profile in) {
    switch (in) {
    case ROC_RESAMPLER_PROFILE_DISABLE:
        return true;

    case ROC_RESAMPLER_PROFILE_LOW:
        out = audio::ResamplerProfile_Low;
        return true;

    case ROC_RESAMPLER_PROFILE_DEFAULT:
    case ROC_RESAMPLER_PROFILE_MEDIUM:
        out = audio::ResamplerProfile_Medium;
        return true;

    case ROC_RESAMPLER_PROFILE_HIGH:
        out = audio::ResamplerProfile_High;
        return true;

    default:
        break;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool packet_encoding_from_user(rtp::PayloadType& out, roc_packet_encoding in) {
    switch (in) {
    case ROC_PACKET_ENCODING_AVP_L16_MONO:
        out = rtp::PayloadType_L16_Mono;
        return true;

    case ROC_PACKET_ENCODING_AVP_L16_STEREO:
        out = rtp::PayloadType_L16_Stereo;
        return true;

    default:
        break;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool fec_encoding_from_user(packet::FecScheme& out, roc_fec_encoding in) {
    switch (in) {
    case ROC_FEC_ENCODING_DISABLE:
        out = packet::FEC_None;
        return true;

    case ROC_FEC_ENCODING_DEFAULT:
    case ROC_FEC_ENCODING_RS8M:
        out = packet::FEC_ReedSolomon_M8;
        return true;

    case ROC_FEC_ENCODING_LDPC_STAIRCASE:
        out = packet::FEC_LDPC_Staircase;
        return true;

    default:
        break;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool interface_from_user(address::Interface& out, const roc_interface& in) {
    switch (in) {
    case ROC_INTERFACE_AUDIO_SOURCE:
        out = address::Iface_AudioSource;
        return true;

    case ROC_INTERFACE_AUDIO_REPAIR:
        out = address::Iface_AudioRepair;
        return true;

    case ROC_INTERFACE_AUDIO_CONTROL:
        out = address::Iface_AudioControl;
        return true;

    default:
        break;
    }

    return false;
}

ROC_ATTR_NO_SANITIZE_UB
bool proto_from_user(address::Protocol& out, const roc_protocol& in) {
    switch (in) {
    case ROC_PROTO_RTSP:
        out = address::Proto_RTSP;
        return true;

    case ROC_PROTO_RTP:
        out = address::Proto_RTP;
        return true;

    case ROC_PROTO_RTP_RS8M_SOURCE:
        out = address::Proto_RTP_RS8M_Source;
        return true;

    case ROC_PROTO_RS8M_REPAIR:
        out = address::Proto_RS8M_Repair;
        return true;

    case ROC_PROTO_RTP_LDPC_SOURCE:
        out = address::Proto_RTP_LDPC_Source;
        return true;

    case ROC_PROTO_LDPC_REPAIR:
        out = address::Proto_LDPC_Repair;
        return true;

    case ROC_PROTO_RTCP:
        out = address::Proto_RTCP;
        return true;

    default:
        break;
    }

    return false;
}

bool proto_to_user(roc_protocol& out, address::Protocol in) {
    switch (in) {
    case address::Proto_RTSP:
        out = ROC_PROTO_RTSP;
        return true;

    case address::Proto_RTP:
        out = ROC_PROTO_RTP;
        return true;

    case address::Proto_RTP_RS8M_Source:
        out = ROC_PROTO_RTP_RS8M_SOURCE;
        return true;

    case address::Proto_RS8M_Repair:
        out = ROC_PROTO_RS8M_REPAIR;
        return true;

    case address::Proto_RTP_LDPC_Source:
        out = ROC_PROTO_RTP_LDPC_SOURCE;
        return true;

    case address::Proto_LDPC_Repair:
        out = ROC_PROTO_LDPC_REPAIR;
        return true;

    case address::Proto_RTCP:
        out = ROC_PROTO_RTCP;
        return true;

    default:
        break;
    }

    return false;
}

} // namespace api
} // namespace roc
