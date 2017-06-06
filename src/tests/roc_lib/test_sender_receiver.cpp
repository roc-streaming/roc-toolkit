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

    void setup() {
        memset(&conf, 0, sizeof(roc_config));
        conf.options = ROC_API_CONF_RESAMPLER_OFF | ROC_API_CONF_INTERLEAVER_OFF;
        conf.FEC_scheme = roc_config::ReedSolomon2m;
        conf.samples_per_packet = (unsigned int)packet_len;
        conf.n_source_packets = 20;
        conf.n_repair_packets = 10;
        conf.latency = 640;
        conf.timeout = packet_len*100;
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
    // conf.options |= ROC_API_CONF_DISABLE_TIMING;
    roc_sender *sndr = roc_sender_new(&conf);

    CHECK(roc_receiver_bind(recv, recv_address));
    CHECK(roc_sender_bind(sndr, recv_address));

    const size_t sz = packet_len*120;
    float s2send[packet_len*120];
    float s2recv[packet_len*120];

    float sval = -1;
    const float sstep = 1./32768.;
    for (size_t i = 0; i < sz; ++i) {
        s2send[i] = sval;
        sval += sstep;
        if (sval > 1) {
            sval = -1;
        }
    }

    roc_sender_write(sndr, s2send, sz);
    CHECK(roc_receiver_read(recv, s2recv, sz) == sz);

    size_t first_sample = 0;
    for(size_t i = 0; i < sz; ++i) {
        if (first_sample == 0 && fabs(s2recv[i]) < 1e-9) {
            continue;
        } else if (first_sample == 0) {
            first_sample = i;
        }
        if (fabs(s2send[i - first_sample] - s2recv[i]) > 1e-9) {
            char sbuff[64];
            snprintf(sbuff, sizeof(sbuff), "Failed comparing samples #%lu\n", i);
            FAIL(sbuff);
        }
    }

    roc_receiver_delete(recv);
    roc_sender_delete(sndr);    
}

} // namespace test
} // namespace roc
