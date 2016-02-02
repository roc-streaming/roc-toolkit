/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/log.h"
#include "roc_datagram/address_to_str.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_audio/sample_buffer_queue.h"
#include "roc_pipeline/receiver.h"
#include "roc_rtp/parser.h"
#include "roc_sndio/writer.h"
#include "roc_netio/transceiver.h"
#include "roc_netio/inet_address.h"

#include "roc_recv/cmdline.h"

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

    datagram::Address addr;
    if (!netio::parse_address(args.inputs[0], addr)) {
        roc_log(LogError, "can't parse address: %s", args.inputs[0]);
        return 1;
    }

    pipeline::ReceiverConfig config;
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
        if (!check_range(
              "nbsrc", args.nbsrc_arg, 3, ROC_CONFIG_MAX_FEC_BLOCK_DATA_PACKETS)) {
            return 1 ;
        }
        config.fec.n_source_packets = (size_t)args.nbsrc_arg;
    }
    if (args.nbrpr_given) {
        if (config.fec.codec != roc::fec::NoCodec) {
            roc_log(LogError, "`--nbrpr' option should not be used when --fec=none");
            return 1;
        }
        if (!check_range("nbrpr", args.nbrpr_arg, 1, (int)config.fec.n_source_packets)) {
            return 1 ;
        }
        config.fec.n_repair_packets = (size_t)args.nbrpr_arg;
    }
    if (args.resampling_arg == resampling_arg_yes) {
        config.options |= pipeline::EnableResampling;
    }
    if (args.timing_arg == timing_arg_yes) {
        config.options |= pipeline::EnableTiming;
    }
    if (args.oneshot_flag) {
        config.options |= pipeline::EnableOneshot;
    }
    if (args.beep_flag) {
        config.options |= pipeline::EnableBeep;
    }
    if (args.rate_given) {
        if (!check_ge("rate", args.rate_arg, 1)) {
            return 1;
        }
        config.sample_rate = (size_t)args.rate_arg;
    }
    if (args.session_timeout_given) {
        if (!check_ge("session-timeout", args.session_timeout_arg, 0)) {
            return 1;
        }
        config.session_timeout = (size_t)args.session_timeout_arg;
    }
    if (args.session_latency_given) {
        if (!check_ge("session-latency", args.session_latency_arg, 0)) {
            return 1;
        }
        config.session_latency = (size_t)args.session_latency_arg;
    }
    if (args.output_latency_given) {
        if (!check_ge("output-latency", args.output_latency_arg, 0)) {
            return 1;
        }
        config.output_latency = (size_t)args.output_latency_arg;
    }
    if (args.output_frame_given) {
        if (!check_ge("output-frame", args.output_frame_arg, 0)) {
            return 1;
        }
        config.samples_per_tick = (size_t)args.output_frame_arg;
    }
    if (args.resampler_frame_given) {
        if (!check_ge("resampler-frame", args.resampler_frame_arg, 0)) {
            return 1;
        }
        config.samples_per_resampler_frame = (size_t)args.resampler_frame_arg;
    }

    datagram::DatagramQueue dgm_queue;
    audio::SampleBufferQueue sample_queue;
    rtp::Parser rtp_parser;

    netio::Transceiver trx;
    if (!trx.add_udp_receiver(addr, dgm_queue)) {
        roc_log(LogError, "can't register udp receiver: %s",
                datagram::address_to_str(addr).c_str());
        return 1;
    }

    pipeline::Receiver receiver(dgm_queue, sample_queue, config);
    receiver.add_port(addr, rtp_parser);

    sndio::Writer writer(sample_queue, config.channels, config.sample_rate);
    if (!writer.open(args.output_arg, args.type_arg)) {
        roc_log(LogError, "can't open output file/device: %s %s", args.output_arg,
                args.type_arg);
        return 1;
    }

    trx.start();

    writer.start();

    receiver.start();
    receiver.join();

    writer.join();

    trx.stop();
    trx.join();

    return 0;
}
