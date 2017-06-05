/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include <string.h>
#include "roc/sender.h"
#include "roc/receiver.h"

namespace roc {
namespace test {

TEST_GROUP(api) {

    const char* recv_address = "127.0.0.1:6000";
    roc_config conf;

    void setup() {
        memset(&conf.options, 0, sizeof(roc_config));
        conf.samples_per_packet = 320;
        conf.n_source_packets = 20;
        conf.n_repair_packets = 10;
    }
}

TEST(api, open) {
    roc_receiver *recv = roc_receiver_new(&conf);
    roc_sender *sndr = roc_sender_new(&conf);

    CHECK(roc_receiver_bind(recv, recv_address));
    CHECK(roc_sender_bind(sndr, recv_address));

    roc_receiver_delete(recv);
    roc_sender_delete(sndr);
}

}
}