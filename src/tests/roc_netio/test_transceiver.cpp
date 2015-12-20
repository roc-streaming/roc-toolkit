/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/log.h"
#include "roc_netio/transceiver.h"

#include "test_datagram_blocking_queue.h"

namespace roc {
namespace test {

using namespace netio;
using namespace datagram;

TEST_GROUP(transceiver) {
    Address make_address(int number){ Address addr;
        addr.ip[0] = 127;
        addr.ip[1] = 0;
        addr.ip[2] = 0;
        addr.ip[3] = 1;
        addr.port = port_t(10000 + number);
        return addr;
    }
};

TEST(transceiver, no_thread) {
    Transceiver trx;
}

TEST(transceiver, start_stop) {
    Transceiver trx;

    trx.start();

    trx.stop();
    trx.join();
}

TEST(transceiver, stop_start) {
    Transceiver trx;

    trx.stop();

    trx.start();
    trx.join();
}

TEST(transceiver, add_no_thread) {
    DatagramBlockingQueue queue;

    Address tx_addr = make_address(1);
    Address rx_addr = make_address(2);

    Transceiver trx;
    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));
}

TEST(transceiver, add_start_stop) {
    DatagramBlockingQueue queue;

    Address tx_addr = make_address(1);
    Address rx_addr = make_address(2);

    Transceiver trx;
    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));

    trx.start();

    trx.stop();
    trx.join();
}

} // namespace test
} // namespace roc
