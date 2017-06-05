/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc/receiver.h"

#include "roc_audio/sample_buffer_queue.h"
#include "roc_core/log.h"
#include "roc_core/math.h"
#include "roc_datagram/address_to_str.h"
#include "roc_datagram/datagram_queue.h"
#include "roc_netio/inet_address.h"
#include "roc_netio/transceiver.h"
#include "roc_pipeline/receiver.h"

using namespace roc;

namespace {

bool make_receiver_config(pipeline::ReceiverConfig& out, const roc_config* in) {
    out = pipeline::ReceiverConfig(pipeline::EnableResampling);

    if (in->options & ROC_API_CONF_DISABLE_FEC) {
        out.fec.codec = fec::NoCodec;
    } else if (in->options & ROC_API_CONF_LDPC_CODE) {
        out.fec.codec = fec::LDPCStaircase;
    } else {
        out.fec.codec = fec::ReedSolomon2m;
    }

    out.fec.n_source_packets = in->n_source_packets;
    out.fec.n_repair_packets = in->n_repair_packets;

    return true;
}

} // anonymous

struct roc_receiver {
    roc_receiver(const pipeline::ReceiverConfig& config)
        : server_(dgm_queue_, sample_queue_, config)
        , buffer_pos_(0) {
    }

    ~roc_receiver() {
        server_.stop();
        server_.join();

        trx_.stop();
        trx_.join();
    }

    bool bind(const char* address) {
        datagram::Address addr;
        if (!netio::parse_address(address, addr)) {
            roc_log(LogError, "can't parse address: %s", address);
            return false;
        }

        if (!trx_.add_udp_receiver(addr, dgm_queue_)) {
            roc_log(LogError, "can't register udp receiver: %s",
                    datagram::address_to_str(addr).c_str());
            return false;
        }

        server_.add_port(addr, pipeline::Proto_RTP);

        trx_.start();
        server_.start();

        return true;
    }

    ssize_t read(float* samples, const size_t n_samples) {
        size_t received_num = 0;

        while (received_num < n_samples) {
            if (!buffer_) {
                buffer_ = sample_queue_.read();

                if (!buffer_) {
                    roc_log(LogInfo, "roc_receiver: got empty buffer, exiting");
                    return -1;
                }
            }

            const size_t cur_buff_num =
                ROC_MIN(buffer_.size() - buffer_pos_, n_samples - received_num);

            memcpy(&samples[received_num], buffer_.data() + buffer_pos_,
                   cur_buff_num * sizeof(packet::sample_t));

            received_num += cur_buff_num;
            buffer_pos_ += cur_buff_num;

            if (buffer_pos_ == buffer_.size()) {
                buffer_pos_ = 0;
                buffer_ = audio::ISampleBufferConstSlice();
            }
        }

        return (ssize_t)received_num;
    }

private:
    datagram::DatagramQueue dgm_queue_;
    audio::SampleBufferQueue sample_queue_;

    netio::Transceiver trx_;

    pipeline::Receiver server_;

    audio::ISampleBufferConstSlice buffer_;
    size_t buffer_pos_;
};

roc_receiver* roc_receiver_new(const roc_config* config) {
    pipeline::ReceiverConfig c;

    if (!make_receiver_config(c, config)) {
        return NULL;
    }

    roc_log(LogInfo, "C API: create roc_receiver");
    return new roc_receiver(c);
}

void roc_receiver_delete(roc_receiver* receiver) {
    roc_panic_if(receiver == NULL);

    roc_log(LogInfo, "C API: delete receiver");
    delete receiver;
}

bool roc_receiver_bind(roc_receiver* receiver, const char* address) {
    roc_panic_if(receiver == NULL);
    roc_panic_if(address == NULL);

    roc_log(LogInfo, "C API: receiver bind to \"%s\"", address);
    return receiver->bind(address);
}

ssize_t
roc_receiver_read(roc_receiver* receiver, float* samples, const size_t n_samples) {
    roc_panic_if(receiver == NULL);
    roc_panic_if(samples == NULL && n_samples != 0);

    return receiver->read(samples, n_samples);
}
