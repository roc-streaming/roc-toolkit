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
#include "roc_core/signal_handler.h"
#include "roc_netio/transceiver.h"
#include "roc_packet/address_to_str.h"
#include "roc_packet/parse_address.h"
#include "roc_pipeline/sender.h"
#include "roc_sndio/init.h"
#include "roc_sndio/recorder.h"

#include "roc_send/cmdline.h"

using namespace roc;

namespace {

enum { MaxPacketSize = 2048, MaxFrameSize = 65 * 1024 };

} // namespace

int main(int argc, char** argv) {
    core::SignalHandler sig_handler;

    gengetopt_args_info args;

    const int code = cmdline_parser(argc, argv, &args);
    if (code != 0) {
        return code;
    }

    core::Logger::instance().set_level(LogLevel(LogError + args.verbose_given));

    sndio::init();

    pipeline::SenderConfig config;

    if (args.source_given) {
        if (!packet::parse_address(args.source_arg, config.source_port.address)) {
            roc_log(LogError, "can't parse remote source address: %s", args.source_arg);
            return 1;
        }
    }

    if (args.repair_given) {
        if (!packet::parse_address(args.repair_arg, config.repair_port.address)) {
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
        config.source_port.protocol = pipeline::Proto_RTP;
        config.repair_port.protocol = pipeline::Proto_RTP;
        break;

    case fec_arg_rs:
        config.fec.codec = fec::ReedSolomon8m;
        config.source_port.protocol = pipeline::Proto_RTP_RSm8_Source;
        config.repair_port.protocol = pipeline::Proto_RSm8_Repair;
        break;

    case fec_arg_ldpc:
        config.fec.codec = fec::LDPCStaircase;
        config.source_port.protocol = pipeline::Proto_RTP_LDPC_Source;
        config.repair_port.protocol = pipeline::Proto_LDPC_Repair;
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

    config.interleaving = (args.interleaving_arg == interleaving_arg_yes);

    core::HeapAllocator allocator;

    core::BufferPool<uint8_t> byte_buffer_pool(allocator, MaxPacketSize, 1);
    core::BufferPool<audio::sample_t> sample_buffer_pool(allocator, MaxFrameSize, 1);
    packet::PacketPool packet_pool(allocator, 1);

    size_t sample_rate = 0;
    if (args.rate_given) {
        if (args.rate_arg <= 0) {
            roc_log(LogError, "invalid --rate: should be > 0");
            return 1;
        }
        sample_rate = (size_t)args.rate_arg;
    }

    sndio::Recorder recorder(sample_buffer_pool, config.channels,
                             config.samples_per_packet, sample_rate);

    if (!recorder.open(args.input_arg, args.type_arg)) {
        roc_log(LogError, "can't open input file/device: %s %s", args.input_arg,
                args.type_arg);
        return 1;
    }
    config.timing = recorder.is_file();

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

    pipeline::Sender sender(config, *udp_sender, *udp_sender, format_map, packet_pool,
                            byte_buffer_pool, allocator);
    if (!sender.valid()) {
        roc_log(LogError, "can't create sender pipeline");
        return 1;
    }

    if (!trx.start()) {
        roc_log(LogError, "can't start transceiver");
        return 1;
    }

    int status = 1;

    if (recorder.start(sender)) {
        recorder.join();
        status = 0;
    } else {
        roc_log(LogError, "can't start recorder");
    }

    trx.stop();
    trx.join();

    trx.remove_port(local_addr);

    return status;
}
