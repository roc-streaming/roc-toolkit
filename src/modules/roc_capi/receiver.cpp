/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_capi/receiver.h"

#include "roc_core/math.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_rtp/parser.h"
#include "roc_datagram/address_to_str.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_audio/sample_buffer_queue.h"
#include "roc_pipeline/server.h"
#include "roc_rtp/parser.h"
#include "roc_netio/transceiver.h"
#include "roc_netio/inet_address.h"


using namespace roc;

struct roc_receiver
{
    roc_receiver() :
        server(dgm_queue, sample_queue, config)
    {}
    ~roc_receiver(){}

    pipeline::ServerConfig config;
    datagram::DatagramQueue dgm_queue;
    audio::SampleBufferQueue sample_queue;
    rtp::Parser rtp_parser;
    netio::Transceiver trx;
    pipeline::Server server;
    audio::ISampleBufferConstSlice buffer;
    size_t buffer_cntr;
};

roc_receiver* roc_receiver_open(const char *address)
{
    datagram::Address addr;
    if (!netio::parse_address(address, addr)) {
        roc_log(LOG_ERROR, "can't parse address: %s", address);
        return NULL;
    }

    core::ScopedPtr<roc_receiver> receiver(new roc_receiver());
    if (!receiver) {
        return NULL;
    }

    receiver->config.options = 0;
    receiver->config.options |= pipeline::EnableFEC |
                            pipeline::EnableResampling | pipeline::EnableBeep;

    if (!receiver->trx.add_udp_receiver(addr, receiver->dgm_queue)) {
        roc_log(LOG_ERROR, "can't register udp receiver: %s",
                datagram::address_to_str(addr).c_str());
        return NULL;
    }

    receiver->buffer_cntr = 0;
    receiver->server.add_port(addr, receiver->rtp_parser);

    receiver->trx.start();
    receiver->server.start();

    return receiver.release();
}

void roc_receiver_close(roc_receiver *receiver)
{
    delete receiver;
}

ssize_t roc_receiver_read(roc_receiver *receiver, float *samples, const size_t n_samples)
{
    size_t received_num;
    for(received_num = 0; received_num < n_samples;){
        if(!receiver->buffer){
            audio::ISampleBufferReader& input = receiver->sample_queue ;
            receiver->buffer = input.read();

            if (!receiver->buffer) {
                roc_log(LOG_DEBUG, "roc_receiver: got empty buffer, exiting");
                return -1;
            }
        }

        // How many samples we're to take from current slice.
        const size_t cur_buff_num = ROC_MIN(receiver->buffer.size()-receiver->buffer_cntr,
                                        n_samples-received_num);
        memcpy(&samples[received_num],
                            receiver->buffer.data()+receiver->buffer_cntr,
                            cur_buff_num*sizeof(roc::packet::sample_t));
        received_num += cur_buff_num;
        receiver->buffer_cntr += cur_buff_num;

        if (receiver->buffer_cntr == receiver->buffer.size()) {
            receiver->buffer_cntr = 0;
            receiver->buffer = audio::ISampleBufferConstSlice();
        }
    }

    return (ssize_t)received_num;
}
