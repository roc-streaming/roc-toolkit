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
#include "roc_pipeline/server.h"
#include "roc_rtp/parser.h"
#include "roc_sndio/writer.h"
#include "roc_netio/transceiver.h"
#include "roc_netio/inet_address.h"

#include "roc_recv/cmdline.h"

using namespace roc;

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

    core::set_log_level(LogLevel(LOG_ERROR + args.verbose_given));

    datagram::Address addr;
    if (!netio::parse_address(args.inputs[0], addr)) {
        roc_log(LOG_ERROR, "can't parse address: %s", args.inputs[0]);
        return 1;
    }

    pipeline::ServerConfig config;
    if (!args.no_fec_flag) {
        config.options |= pipeline::EnableFEC;
    }
    if (!args.no_resampling_flag) {
        config.options |= pipeline::EnableResampling;
    }
    if (!args.no_timing_flag) {
        config.options |= pipeline::EnableTiming;
    }
    if (args.beep_flag) {
        config.options |= pipeline::EnableBeep;
    }

    datagram::DatagramQueue dgm_queue;
    audio::SampleBufferQueue sample_queue;
    rtp::Parser rtp_parser;

    netio::Transceiver trx;
    if (!trx.add_udp_receiver(addr, dgm_queue)) {
        roc_log(LOG_ERROR, "can't register receiving address: %s",
                datagram::address_to_str(addr).c_str());
        return 1;
    }

    pipeline::Server server(dgm_queue, sample_queue, config);
    server.add_port(addr, rtp_parser);

    sndio::Writer writer(sample_queue);
    if (!writer.open(args.output_arg, args.type_arg)) {
        roc_log(LOG_ERROR, "can't open output writer: name=%s type=%s", args.output_arg,
                args.type_arg);
        return 1;
    }

    writer.start();
    server.start();
    trx.start();

    // TODO: wait ^C

    trx.join();
    server.join();
    writer.join();

    return 0;
}
