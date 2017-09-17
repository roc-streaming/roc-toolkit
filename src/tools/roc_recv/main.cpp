/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/parse_address.h"
#include "roc_pipeline/receiver.h"
#include "roc_sndio/init.h"
#include "roc_sndio/player.h"

#include "roc_recv/cmdline.h"

using namespace roc;

namespace {

enum { MaxPacketSize = 2048, MaxFrameSize = 65 * 1024 };

bool check_ge(const char* option, int value, int min_value) {
    if (value < min_value) {
        roc_log(LogError, "invalid `--%s=%d': should be >= %d", option, value, min_value);
        return false;
    }
    return true;
}

} // namespace

int main(int argc, char** argv) {
    gengetopt_args_info args;

    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }

    core::set_log_level(LogLevel(LogError + args.verbose_given));

    sndio::init();

    pipeline::PortConfig source_port;
    if (args.source_given) {
        if (!packet::parse_address(args.source_arg, source_port.address)) {
            roc_log(LogError, "can't parse source address: %s", args.source_arg);
            return 1;
        }
    }

    pipeline::PortConfig repair_port;
    if (args.repair_given) {
        if (!packet::parse_address(args.repair_arg, repair_port.address)) {
            roc_log(LogError, "can't parse repair address: %s", args.repair_arg);
            return 1;
        }
    }

    pipeline::ReceiverConfig config;

    switch ((unsigned)args.fec_arg) {
    case fec_arg_none:
        config.default_session.fec.codec = fec::NoCodec;
        source_port.protocol = pipeline::Proto_RTP;
        repair_port.protocol = pipeline::Proto_RTP;
        break;

    case fec_arg_rs:
        config.default_session.fec.codec = fec::ReedSolomon8m;
        source_port.protocol = pipeline::Proto_RTP_RSm8_Source;
        repair_port.protocol = pipeline::Proto_RSm8_Repair;
        break;

    case fec_arg_ldpc:
        config.default_session.fec.codec = fec::LDPCStaircase;
        source_port.protocol = pipeline::Proto_RTP_LDPC_Source;
        repair_port.protocol = pipeline::Proto_LDPC_Repair;
        break;

    default:
        break;
    }

    if (args.nbsrc_given) {
        if (config.default_session.fec.codec != fec::NoCodec) {
            roc_log(LogError, "`--nbsrc' option should not be used when --fec=none)");
            return 1;
        }
        config.default_session.fec.n_source_packets = (size_t)args.nbsrc_arg;
    }

    if (args.nbrpr_given) {
        if (config.default_session.fec.codec != fec::NoCodec) {
            roc_log(LogError, "`--nbrpr' option should not be used when --fec=none");
            return 1;
        }
        config.default_session.fec.n_repair_packets = (size_t)args.nbrpr_arg;
    }

    config.default_session.resampling = (args.resampling_arg == resampling_arg_yes);
    config.timing = (args.timing_arg == timing_arg_yes);
    config.default_session.beep = args.beep_flag;

    if (args.rate_given) {
        if (!check_ge("rate", args.rate_arg, 1)) {
            return 1;
        }
        config.sample_rate = (size_t)args.rate_arg;
    }

    if (args.timeout_given) {
        if (!check_ge("timeout", args.timeout_arg, 0)) {
            return 1;
        }
        config.default_session.timeout = (packet::timestamp_t)args.timeout_arg;
    }

    if (args.latency_given) {
        if (!check_ge("latency", args.latency_arg, 0)) {
            return 1;
        }
        config.default_session.latency = (packet::timestamp_t)args.latency_arg;
    }

    if (args.resampler_window_given) {
        if (!check_ge("resampler-window", args.resampler_window_arg, 0)) {
            return 1;
        }
        config.default_session.resampler.window_size = (size_t)args.resampler_window_arg;
    }

    if (args.resampler_frame_given) {
        if (!check_ge("resampler-frame", args.resampler_frame_arg, 0)) {
            return 1;
        }
        config.default_session.resampler.frame_size = (size_t)args.resampler_frame_arg;
    }

    core::HeapAllocator allocator;

    core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxPacketSize, 1);
    core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxFrameSize, 1);
    packet::PacketPool packet_pool(allocator, 1);

    rtp::FormatMap format_map;

    pipeline::Receiver receiver(config, format_map, packet_pool, byte_buffer_pool,
                                sample_buffer_pool, allocator);
    if (!receiver.valid()) {
        roc_log(LogError, "can't create receiver pipeline");
        return 1;
    }

    netio::Transceiver trx(packet_pool, byte_buffer_pool, allocator);
    if (!trx.valid()) {
        roc_log(LogError, "can't create network transceiver");
        return 1;
    }

    if (!trx.add_udp_receiver(source_port.address, receiver)) {
        roc_log(LogError, "can't register udp receiver: %s",
                packet::address_to_str(source_port.address).c_str());
        return 1;
    }
    if (!receiver.add_port(source_port)) {
        roc_log(LogError, "can't add udp port: %s",
                packet::address_to_str(source_port.address).c_str());
        return 1;
    }

    if (config.default_session.fec.codec != fec::NoCodec) {
        if (!trx.add_udp_receiver(repair_port.address, receiver)) {
            roc_log(LogError, "can't register udp receiver: %s",
                    packet::address_to_str(repair_port.address).c_str());
            return 1;
        }
        if (!receiver.add_port(repair_port)) {
            roc_log(LogError, "can't add udp port: %s",
                    packet::address_to_str(repair_port.address).c_str());
            return 1;
        }
    }

    sndio::Player player(receiver, sample_buffer_pool, allocator, args.oneshot_flag,
                         config.channels, config.sample_rate);

    if (!player.open(args.output_arg, args.type_arg)) {
        roc_log(LogError, "can't open output file or device: %s %s", args.output_arg,
                args.type_arg);
        return 1;
    }

    trx.start();

    player.start();
    player.join();

    trx.stop();
    trx.join();

    return 0;
}
