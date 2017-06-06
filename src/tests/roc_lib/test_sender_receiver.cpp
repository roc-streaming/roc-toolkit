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
#include "roc/log.h"
#include "roc_core/stddefs.h"
#include "roc_datagram/address_to_str.h"
#include "roc_netio/inet_address.h"
#include "roc_netio/transceiver.h"
#include "roc_core/random.h"

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
    static const size_t packet_len = 320;
    static const size_t packet_num = 150;

    class proxy : public datagram::IDatagramWriter
    {
    public:
        proxy(datagram::IDatagramWriter& datagram_writer, const char *src_addr,
                const char *dst_addr, const size_t loss_rate)
            : writer_(datagram_writer)
            , loss_rate_(loss_rate)
        {
            CHECK(netio::parse_address(src_addr, src_addr_));
            CHECK(netio::parse_address(dst_addr, dst_addr_));
        }
        virtual ~proxy()
        {}

        virtual void write(const datagram::IDatagramPtr& ptr) {
            if (core::random(100) < loss_rate_) {
                return;
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
    };

    static const size_t sz = packet_len*packet_num;
    float s2send[sz];
    float s2recv[sz];

    void setup() {
        memset(&conf, 0, sizeof(roc_config));
        conf.options = ROC_API_CONF_RESAMPLER_OFF | ROC_API_CONF_INTERLEAVER_OFF;
        conf.FEC_scheme = roc_config::ReedSolomon2m;
        conf.samples_per_packet = (unsigned int)packet_len;
        conf.n_source_packets = 20;
        conf.n_repair_packets = 10;
        conf.latency = 640;
        conf.timeout = packet_len*100;

        const float sstep = 1./32768.;
        float sval = -1 + sstep;
        for (size_t i = 0; i < sz; ++i) {
            s2send[i] = sval;
            sval += sstep;
            if (sval >= 1) {
                sval = -1 + sstep;
            }
        }
    }

    void check_sample_arrays(float *original, float *received, const size_t len){
        size_t first_sample = 0;
        for(size_t i = 0; i < len; ++i) {
            if (first_sample == 0 && fabs(double(received[i])) < 1e-9) {
                continue;
            } else if (first_sample == 0) {
                first_sample = i;
            }
            if (fabs(double(original[i - first_sample] - received[i])) > 1e-9) {
                char sbuff[64];
                snprintf(sbuff, sizeof(sbuff), "Failed comparing samples #%lu\n", i);
                FAIL(sbuff);
            }
        }
    }
};

TEST(sender_receiver, open) {
    roc_receiver* recv = roc_receiver_new(&conf);
    roc_sender* sndr = roc_sender_new(&conf);

    CHECK(roc_receiver_bind(recv, recv_address));
    CHECK(roc_sender_bind(sndr, recv_address));

    roc_receiver_delete(recv);
    roc_sender_delete(sndr);
}

TEST(sender_receiver, single_bunch) {

    roc_receiver *recv = roc_receiver_new(&conf);
    roc_sender *sndr = roc_sender_new(&conf);

    CHECK(roc_receiver_bind(recv, recv_address));
    CHECK(roc_sender_bind(sndr, recv_address));

    roc_sender_write(sndr, s2send, sz);
    CHECK(roc_receiver_read(recv, s2recv, sz) == sz);

    check_sample_arrays(s2send, s2recv, sz);

    roc_receiver_delete(recv);
    roc_sender_delete(sndr);
}

TEST(sender_receiver, test_fec) {
    netio::Transceiver trx;
    const char *relay_addr_str = "127.0.0.1:5998";
    const char *src_addr_str = "127.0.0.1:5999";
    proxy relay(trx.udp_sender(), src_addr_str, recv_address, 5);
    roc_sender *sndr = roc_sender_new(&conf);

    conf.latency = packet_len * 40;
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
    CHECK(roc_sender_bind(sndr, relay_addr_str));

    roc_sender_write(sndr, s2send, sz);
    CHECK(roc_receiver_read(recv, s2recv, sz) == sz);

    check_sample_arrays(s2send, s2recv, sz);

    trx.stop();
    trx.join();

    roc_receiver_delete(recv);
    roc_sender_delete(sndr);
}

} // namespace test
} // namespace roc
