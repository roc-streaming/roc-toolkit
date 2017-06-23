/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_buffer_queue.h"
#include "roc_core/log.h"
#include "roc_datagram/address_to_str.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_netio/inet_address.h"
#include "roc_netio/transceiver.h"
#include "roc_pipeline/sender.h"
#include "roc_sndio/reader.h"

#include "roc_send/cmdline.h"

using namespace roc;

namespace {

bool check_ge(const char* option, int value, int min_value) {
    if (value < min_value) {
        roc_log(LogError, "invalid `--%s=%d': should be >= %d", option, value, min_value);
        return false;
    }
    return true;
}

bool check_range(const char* option, int value, int min_value, int max_value) {
    if (value < min_value || value > max_value) {
        roc_log(LogError, "invalid `--%s=%d': should be in range [%d; %d]", option, value,
                min_value, max_value);
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

    if (args.inputs_num != 1) {
        fprintf(stderr, "%s\n", gengetopt_args_info_usage);
        return 1;
    }

    core::set_log_level(LogLevel(LogError + args.verbose_given));

    datagram::Address src_addr;
    if (args.source_given) {
        if (!netio::parse_address(args.source_arg, src_addr)) {
            roc_log(LogError, "can't parse source address: %s", args.source_arg);
            return 1;
        }
    }

    datagram::Address dst_addr;
    if (!netio::parse_address(args.inputs[0], dst_addr)) {
        roc_log(LogError, "can't parse destination address: %s", args.inputs[0]);
        return 1;
    }

    pipeline::SenderConfig config;
    if (args.fec_arg == fec_arg_none) {
        config.fec.codec = roc::fec::NoCodec;
    } else if (args.fec_arg == fec_arg_rs) {
        config.fec.codec = roc::fec::ReedSolomon2m;
    } else if (args.fec_arg == fec_arg_ldpc) {
        config.fec.codec = roc::fec::LDPCStaircase;
    }
    if (args.nbsrc_given) {
        if (config.fec.codec != roc::fec::NoCodec) {
            roc_log(LogError, "`--nbsrc' option should not be used when --fec=none)");
            return 1;
        }
        if (!check_range("nbsrc", args.nbsrc_arg, 3,
                         ROC_CONFIG_MAX_FEC_BLOCK_SOURCE_PACKETS)) {
            return 1;
        }
        config.fec.n_source_packets = (size_t)args.nbsrc_arg;
    }
    if (args.nbrpr_given) {
        if (config.fec.codec != roc::fec::NoCodec) {
            roc_log(LogError, "`--nbrpr' option should not be used when --fec=none");
            return 1;
        }
        if (!check_range("nbrpr", args.nbrpr_arg, 1, (int)config.fec.n_source_packets)) {
            return 1;
        }
        config.fec.n_repair_packets = (size_t)args.nbrpr_arg;
    }
    if (args.interleaving_arg == interleaving_arg_yes) {
        config.options |= pipeline::EnableInterleaving;
    }
    if (args.timing_arg == timing_arg_yes) {
        config.options |= pipeline::EnableTiming;
    }
    if (args.rate_given) {
        if (!check_ge("rate", args.rate_arg, 1)) {
            return 1;
        }
        config.sample_rate = (size_t)args.rate_arg;
    }
    if (args.loss_rate_given) {
        if (!check_range("loss-rate", args.loss_rate_arg, 0, 100)) {
            return 1;
        }
        config.random_loss_rate = (size_t)args.loss_rate_arg;
    }
    if (args.delay_rate_given) {
        if (!check_range("delay-rate", args.delay_rate_arg, 0, 100)) {
            return 1;
        }
        if (!check_ge("delay", args.delay_arg, 0)) {
            return 1;
        }
        config.random_delay_rate = (size_t)args.delay_rate_arg;
        config.random_delay_time = (size_t)args.delay_arg;
    }

    audio::SampleBufferQueue sample_queue;

    sndio::Reader reader(sample_queue, audio::default_buffer_composer(), config.channels,
                         config.samples_per_packet / 2, config.sample_rate);

    if (!reader.open(args.input_arg, args.type_arg)) {
        roc_log(LogError, "can't open input file/device: %s %s", args.input_arg,
                args.type_arg);
        return 1;
    }

    netio::Transceiver trx;
    if (!trx.add_udp_sender(src_addr)) {
        roc_log(LogError, "can't register udp sender: %s",
                datagram::address_to_str(src_addr).c_str());
        return 1;
    }

    pipeline::Sender sender(sample_queue, trx.udp_sender(), trx.udp_composer(), config);

    sender.set_audio_port(src_addr, dst_addr, pipeline::Proto_RTP);
    sender.set_repair_port(src_addr, dst_addr, pipeline::Proto_RTP); // FIXME

    trx.start();

    sender.start();

    reader.start();
    reader.join();

    sender.join();

    trx.join();

    return 0;
}
