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
#include "roc_pipeline/client.h"
#include "roc_rtp/composer.h"
#include "roc_sndio/reader.h"
#include "roc_netio/transceiver.h"
#include "roc_netio/inet_address.h"

#include "roc_send/cmdline.h"

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

    datagram::Address src_addr;
    if (args.source_given) {
        if (!netio::parse_address(args.source_arg, src_addr)) {
            roc_log(LOG_ERROR, "can't parse source address: %s", args.source_arg);
            return 1;
        }
    }

    datagram::Address dst_addr;
    if (!netio::parse_address(args.inputs[0], dst_addr)) {
        roc_log(LOG_ERROR, "can't parse destination address: %s", args.inputs[0]);
        return 1;
    }

    pipeline::ClientConfig config;
    if (args.fec_arg == fec_arg_enable) {
        config.options |= pipeline::EnableFEC;
    }
    if (args.interleaving_arg == interleaving_arg_yes) {
        config.options |= pipeline::EnableInterleaving;
    }
    if (args.timing_arg == timing_arg_yes) {
        config.options |= pipeline::EnableTiming;
    }
    if (args.loss_rate_given) {
        config.random_loss_rate = (size_t)args.loss_rate_arg;
    }
    if (args.delay_rate_given) {
        config.random_delay_rate = (size_t)args.delay_rate_arg;
        config.random_delay_time = (size_t)args.delay_arg;
    }

    audio::SampleBufferQueue sample_queue;
    rtp::Composer rtp_composer;

    sndio::Reader reader(sample_queue);
    if (!reader.open(args.input_arg, args.type_arg)) {
        roc_log(LOG_ERROR, "can't open input file/device: %s %s", args.input_arg,
                args.type_arg);
        return 1;
    }

    netio::Transceiver trx;
    if (!trx.add_udp_sender(src_addr)) {
        roc_log(LOG_ERROR, "can't register udp sender: %s",
                datagram::address_to_str(src_addr).c_str());
        return 1;
    }

    pipeline::Client client(sample_queue, trx.udp_sender(), trx.udp_composer(),
                            rtp_composer, config);

    client.set_sender(src_addr);
    client.set_receiver(dst_addr);

    trx.start();

    client.start();

    reader.start();
    reader.join();

    client.join();

    trx.join();

    return 0;
}
