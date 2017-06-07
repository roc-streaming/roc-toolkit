/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc/receiver.h"
#include "roc/sender.h"
#include "roc_core/log.h"
#include "roc/log.h"
#include "roc_core/stddefs.h"
#include "roc_datagram/address_to_str.h"
#include "roc_netio/inet_address.h"
#include "roc_netio/transceiver.h"
#include "roc_core/random.h"
#include "roc_core/thread.h"

#include <stdio.h>
#include <stdio.h>

#include <unistd.h>

namespace roc {
namespace test {

namespace {

const char* recv_address = "127.0.0.1:6000";

} // namespace

TEST_GROUP(sender_receiver) {
    roc_config conf;
    static const size_t packet_len = 640;
    static const size_t packet_num = 100;

    //! Relays samples with losses.
    class proxy : public datagram::IDatagramWriter {
    public:
        proxy(datagram::IDatagramWriter& datagram_writer, const char *src_addr,
                const char *dst_addr, const size_t loss_rate)
            : writer_(datagram_writer)
            , loss_rate_(loss_rate)
            , is_first(true)
        {
            CHECK(netio::parse_address(src_addr, src_addr_));
            CHECK(netio::parse_address(dst_addr, dst_addr_));
        }
        virtual ~proxy()
        {}

        virtual void write(const datagram::IDatagramPtr& ptr) {
            // FIXME: we can't lost first packet with marker so far,
            // otherwise block decoder won't be able to find the beginning 
            // of the block. Will be removed when roc will support FECFRAME.
            if (!is_first && core::random(100) < loss_rate_) {
                return;
            } else if (is_first) {
                is_first = false;
            }
            ptr->set_sender(src_addr_);
            ptr->set_receiver(dst_addr_);
            writer_.write(ptr);
        }

    private:
        datagram::Address src_addr_;
        datagram::Address dst_addr_;
        datagram::IDatagramWriter& writer_;

        const size_t loss_rate_;
        bool is_first;
    };

    class sender : public core::Thread {
    public:
        sender(roc_config &config, const char* dst_address,
                float *samples, const size_t len)
            : sndr_(roc_sender_new(&config))
            , samples_(samples)
            , sz_(len)
        {
            roc_sender_bind(sndr_, dst_address);
        }
        virtual ~sender(){
            roc_sender_delete(sndr_);
        }

    private:
        virtual void run(){
            roc_sender_write(sndr_, samples_, sz_);
        }

        roc_sender *sndr_;
        float *samples_;
        const size_t sz_;
    };

    static const size_t total_sz = packet_len*packet_num;
    float s2send[total_sz];
    float s2recv[total_sz];

    void setup() {
        memset(&conf, 0, sizeof(roc_config));
        conf.options = ROC_API_CONF_RESAMPLER_OFF | ROC_API_CONF_INTERLEAVER_OFF;
        conf.FEC_scheme = roc_config::ReedSolomon2m;
        conf.samples_per_packet = (unsigned int)packet_len/2;
        conf.n_source_packets = 10;
        conf.n_repair_packets = 5;
        conf.latency = packet_len*20;
        conf.timeout = packet_len*300;

        const float sstep = 1./32768.;
        float sval = -1 + sstep;
        for (size_t i = 0; i < total_sz; ++i) {
            s2send[i] = sval;
            sval += sstep;
            if (sval >= 1) {
                sval = -1 + sstep;
            }
        }
    }

    void check_sample_arrays(roc_receiver *recv, float *original, const size_t len) {
        float rx_buff[packet_len];
        size_t s_first = 0;
        size_t inner_cntr = 0;
        bool seek_first = true;
        size_t s_last = 0;

        size_t ipacket = 0;
        while(s_last == 0){
            size_t i = 0;
            ipacket++;
            CHECK(roc_receiver_read(recv, rx_buff, packet_len) == (ssize_t)packet_len);
            if (seek_first) {
                for (; i < packet_len && fabs(double(rx_buff[i])) < 1e-9; i++, s_first++)
                    {}
                CHECK(s_first < len);
                if (i < packet_len) {
                    seek_first = false;
                }
            } 
            if (!seek_first) {
                for (; i < packet_len; i++) {
                    if (inner_cntr >= len) {
                        CHECK(fabs(double(rx_buff[i])) < 1e-9);
                        s_last = inner_cntr + s_first;
                        roc_log(LogInfo,
                            "FINISH: s_first: %lu, s_last: %lu, inner_cntr: %lu",
                                s_first, s_last, inner_cntr);
                        break;
                    } else if (fabs(double(original[inner_cntr] - rx_buff[i])) > 1e-9) {
                        char sbuff[256];
                        int sbuff_i = snprintf(sbuff, sizeof(sbuff),
                            "Failed comparing samples #%lu\n\npacket_num: %lu\n",
                                (unsigned long)inner_cntr, (unsigned long)ipacket);
                        snprintf(&sbuff[sbuff_i], sizeof(sbuff)-(size_t)sbuff_i,
                            "original: %f,\treceived: %f\n",
                                (double)original[inner_cntr], (double)rx_buff[i]);
                        FAIL(sbuff);
                    } else {
                        inner_cntr++;
                    }
                }
            }
        }
    }
};

TEST(sender_receiver, single_bunch) {
    roc_receiver *recv = roc_receiver_new(&conf);
    sender sndr(conf, recv_address, s2send, total_sz);

    CHECK(roc_receiver_bind(recv, recv_address));

    

    sndr.start();
    check_sample_arrays(recv, s2send, total_sz);
    sndr.join();


    roc_receiver_delete(recv);
}

#ifdef ROC_TARGET_OPENFEC
TEST(sender_receiver, test_fec) {
    netio::Transceiver trx;
    const char *relay_addr_str = "127.0.0.1:5998";
    const char *src_addr_str = "127.0.0.1:5999";
    proxy relay(trx.udp_sender(), src_addr_str, recv_address, 5);
    sender sndr(conf, relay_addr_str, s2send, total_sz);

    roc_receiver *recv = roc_receiver_new(&conf);

    datagram::Address relay_addr;
    datagram::Address src_addr;
    datagram::Address dst_addr;

    CHECK(netio::parse_address(relay_addr_str, relay_addr));
    CHECK(netio::parse_address(src_addr_str, src_addr));
    CHECK(netio::parse_address(recv_address, dst_addr));

    CHECK(trx.add_udp_sender(src_addr));
    CHECK(trx.add_udp_receiver(relay_addr, relay));

    trx.start();

    CHECK(roc_receiver_bind(recv, recv_address));

    sndr.start();
    check_sample_arrays(recv, s2send, total_sz);
    sndr.join();

    trx.stop();
    trx.join();

    roc_receiver_delete(recv);
}
#endif // ROC_TARGET_OPENFEC

} // namespace test
} // namespace roc
