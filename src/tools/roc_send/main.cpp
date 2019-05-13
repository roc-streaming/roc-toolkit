/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/resampler_profile.h"
#include "roc_core/crash.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/parse_duration.h"
#include "roc_core/scoped_destructor.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/parse_address.h"
#include "roc_pipeline/sender.h"
#include "roc_sndio/pump.h"
#include "roc_sndio/sox_controller.h"
#include "roc_sndio/sox_source.h"

#include "roc_send/cmdline.h"

using namespace roc;

int main(int argc, char** argv) {
    core::CrashHandler crash_handler;

    gengetopt_args_info args;

    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }

    core::ScopedDestructor<gengetopt_args_info*, cmdline_parser_free> args_destructor(
        &args);

    core::Logger::instance().set_level(
        LogLevel(core::DefaultLogLevel + args.verbose_given));

    pipeline::SenderConfig config;

    if (args.packet_length_given) {
        if (!core::parse_duration(args.packet_length_arg, config.packet_length)) {
            roc_log(LogError, "invalid --packet-length");
            return 1;
        }
    }

    size_t max_packet_size = 2048;
    if (args.packet_limit_given) {
        if (args.packet_limit_arg <= 0) {
            roc_log(LogError, "invalid --packet-limit: should be > 0");
            return 1;
        }
        max_packet_size = (size_t)args.packet_limit_arg;
    }

    if (args.frame_size_given) {
        if (args.frame_size_arg <= 0) {
            roc_log(LogError, "invalid --frame-size: should be > 0");
            return 1;
        }
        config.internal_frame_size = (size_t)args.frame_size_arg;
    }

    sndio::SoxController::instance().set_buffer_size(config.internal_frame_size);

    pipeline::PortConfig source_port;
    pipeline::PortConfig repair_port;

    if (args.source_given) {
        if (!packet::parse_address(args.source_arg, source_port.address)) {
            roc_log(LogError, "can't parse remote source address: %s", args.source_arg);
            return 1;
        }
    }

    if (args.repair_given) {
        if (!packet::parse_address(args.repair_arg, repair_port.address)) {
            roc_log(LogError, "can't parse remote repair address: %s", args.repair_arg);
            return 1;
        }
    }

    packet::Address local_addr;
    if (args.local_given) {
        if (!packet::parse_address(args.local_arg, local_addr)) {
            roc_log(LogError, "can't parse local address: %s", args.local_arg);
            return 1;
        }
    } else {
        packet::parse_address(":0", local_addr);
    }

    switch ((unsigned)args.fec_arg) {
    case fec_arg_none:
        config.fec.codec = fec::NoCodec;
        source_port.protocol = pipeline::Proto_RTP;
        repair_port.protocol = pipeline::Proto_RTP;
        break;

    case fec_arg_rs:
        config.fec.codec = fec::ReedSolomon8m;
        source_port.protocol = pipeline::Proto_RTP_RSm8_Source;
        repair_port.protocol = pipeline::Proto_RSm8_Repair;
        break;

    case fec_arg_ldpc:
        config.fec.codec = fec::LDPCStaircase;
        source_port.protocol = pipeline::Proto_RTP_LDPC_Source;
        repair_port.protocol = pipeline::Proto_LDPC_Repair;
        break;

    default:
        break;
    }

    if (args.nbsrc_given) {
        if (config.fec.codec == fec::NoCodec) {
            roc_log(LogError, "--nbsrc can't be used when --fec=none)");
            return 1;
        }
        if (args.nbsrc_arg <= 0) {
            roc_log(LogError, "invalid --nbsrc: should be > 0");
            return 1;
        }
        config.fec.n_source_packets = (size_t)args.nbsrc_arg;
    }

    if (args.nbrpr_given) {
        if (config.fec.codec == fec::NoCodec) {
            roc_log(LogError, "--nbrpr can't be used when --fec=none");
            return 1;
        }
        if (args.nbrpr_arg <= 0) {
            roc_log(LogError, "invalid --nbrpr: should be > 0");
            return 1;
        }
        config.fec.n_repair_packets = (size_t)args.nbrpr_arg;
    }

    config.resampling = !args.no_resampling_flag;

    switch ((unsigned)args.resampler_profile_arg) {
    case resampler_profile_arg_low:
        config.resampler = audio::resampler_profile(audio::ResamplerProfile_Low);
        break;

    case resampler_profile_arg_medium:
        config.resampler = audio::resampler_profile(audio::ResamplerProfile_Medium);
        break;

    case resampler_profile_arg_high:
        config.resampler = audio::resampler_profile(audio::ResamplerProfile_High);
        break;

    default:
        break;
    }

    if (args.resampler_interp_given) {
        if (args.resampler_interp_arg <= 0) {
            roc_log(LogError, "invalid --resampler-interp: should be > 0");
            return 1;
        }
        config.resampler.window_interp = (size_t)args.resampler_interp_arg;
    }

    if (args.resampler_window_given) {
        if (args.resampler_window_arg <= 0) {
            roc_log(LogError, "invalid --resampler-window: should be > 0");
            return 1;
        }
        config.resampler.window_size = (size_t)args.resampler_window_arg;
    }

    sndio::Config source_config;
    source_config.channels = config.input_channels;
    source_config.frame_size = config.internal_frame_size;

    if (args.rate_given) {
        if (args.rate_arg <= 0) {
            roc_log(LogError, "invalid --rate: should be > 0");
            return 1;
        }
        source_config.sample_rate = (size_t)args.rate_arg;
    } else {
        if (!config.resampling) {
            source_config.sample_rate = pipeline::DefaultSampleRate;
        }
    }

    config.interleaving = args.interleaving_flag;
    config.poisoning = args.poisoning_flag;

    core::HeapAllocator allocator;
    core::BufferPool<uint8_t> byte_buffer_pool(allocator, max_packet_size,
                                               args.poisoning_flag);
    core::BufferPool<audio::sample_t> sample_buffer_pool(
        allocator, config.internal_frame_size, args.poisoning_flag);
    packet::PacketPool packet_pool(allocator, args.poisoning_flag);

    sndio::SoxSource source(allocator, source_config);
    if (!source.open(args.driver_arg, args.input_arg)) {
        roc_log(LogError, "can't open input file or device: driver=%s input=%s",
                args.driver_arg, args.input_arg);
        return 1;
    }

    config.timing = !source.has_clock();
    config.input_sample_rate = source.sample_rate();

    rtp::FormatMap format_map;

    netio::Transceiver trx(packet_pool, byte_buffer_pool, allocator);
    if (!trx.valid()) {
        roc_log(LogError, "can't create network transceiver");
        return 1;
    }

    packet::IWriter* udp_sender = trx.add_udp_sender(local_addr);
    if (!udp_sender) {
        roc_log(LogError, "can't create udp sender");
        return 1;
    }

    pipeline::Sender sender(config, source_port, *udp_sender, repair_port, *udp_sender,
                            format_map, packet_pool, byte_buffer_pool, sample_buffer_pool,
                            allocator);
    if (!sender.valid()) {
        roc_log(LogError, "can't create sender pipeline");
        return 1;
    }

    sndio::Pump pump(sample_buffer_pool, source, sender, config.internal_frame_size,
                     sndio::Pump::ModePermanent);
    if (!pump.valid()) {
        roc_log(LogError, "can't create audio pump");
        return 1;
    }

    if (!trx.start()) {
        roc_log(LogError, "can't start transceiver");
        return 1;
    }

    const bool ok = pump.run();

    trx.stop();
    trx.join();

    trx.remove_port(local_addr);

    return ok ? 0 : 1;
}
