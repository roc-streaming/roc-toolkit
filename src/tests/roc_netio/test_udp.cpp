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

namespace {

enum { NumIterations = 20, NumPackets = 10, BufferSize = 125 };

} // namespace

TEST_GROUP(udp) {
    Address make_address(int number){ Address addr;
        addr.ip[0] = 127;
        addr.ip[1] = 0;
        addr.ip[2] = 0;
        addr.ip[3] = 1;
        addr.port = port_t(10000 + number);
        return addr;
    }

    core::IByteBufferConstSlice make_buffer(int number, int base) {
        core::IByteBufferPtr buff = default_buffer_composer().compose();
        CHECK(buff);

        buff->set_size(BufferSize);

        for (int n = 0; n < BufferSize; n++) {
            buff->data()[n] = uint8_t((base * number + n) & 0xff);
        }

        return *buff;
    }

    void send_datagram(
        Transceiver& tx, Address tx_addr, Address rx_addr, int number, int base) {
        //
        IDatagramPtr dgm = tx.udp_composer().compose();
        CHECK(dgm);

        dgm->set_sender(tx_addr);
        dgm->set_receiver(rx_addr);
        dgm->set_buffer(make_buffer(number, base));

        tx.udp_sender().write(dgm);
    }

    void wait_datagram(DatagramBlockingQueue& queue,
                       Address tx_addr,
                       Address rx_addr,
                       int number,
                       int base) {
        //
        IDatagramConstPtr dgm = queue.read();
        CHECK(dgm);

        expect_address(tx_addr, dgm->sender());
        expect_address(rx_addr, dgm->receiver());
        expect_buffer(number, base, dgm->buffer());
    }

    void expect_address(const Address& expected, const Address& actual) {
        LONGS_EQUAL(expected.ip[0], actual.ip[0]);
        LONGS_EQUAL(expected.ip[1], actual.ip[1]);
        LONGS_EQUAL(expected.ip[2], actual.ip[2]);
        LONGS_EQUAL(expected.ip[3], actual.ip[3]);
        LONGS_EQUAL(expected.port, actual.port);
        CHECK(expected == actual);
    }

    void expect_buffer(int number, int base, core::IByteBufferConstSlice actual) {
        core::IByteBufferConstSlice expected = make_buffer(number, base);

        LONGS_EQUAL(expected.size(), actual.size());

        for (size_t n = 0; n < expected.size(); n++) {
            uint8_t val_expected = expected.data()[n];
            uint8_t val_actual = actual.data()[n];

            if (val_expected != val_actual) {
                roc_log(LogError, "unexpected byte at pos %u (datagram # %d):",
                        (unsigned)n, number);

                actual.print();
            }

            LONGS_EQUAL(val_expected, val_actual);
        }
    }
};

TEST(udp, one_sender_one_receiver_single_thread) {
    DatagramBlockingQueue queue;

    Address tx_addr = make_address(1);
    Address rx_addr = make_address(2);

    Transceiver trx;
    CHECK(trx.add_udp_sender(tx_addr));
    CHECK(trx.add_udp_receiver(rx_addr, queue));

    trx.start();

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            send_datagram(trx, tx_addr, rx_addr, p, 77);
        }
        for (int p = 0; p < NumPackets; p++) {
            wait_datagram(queue, tx_addr, rx_addr, p, 77);
        }
    }

    trx.stop();
    trx.join();
}

TEST(udp, one_sender_one_receiver_separate_threads) {
    DatagramBlockingQueue queue;

    Address tx_addr = make_address(1);
    Address rx_addr = make_address(2);

    Transceiver tx;
    CHECK(tx.add_udp_sender(tx_addr));

    Transceiver rx;
    CHECK(rx.add_udp_receiver(rx_addr, queue));

    tx.start();
    rx.start();

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            send_datagram(tx, tx_addr, rx_addr, p, 55);
        }
        for (int p = 0; p < NumPackets; p++) {
            wait_datagram(queue, tx_addr, rx_addr, p, 55);
        }
    }

    tx.stop();
    tx.join();

    rx.stop();
    rx.join();
}

TEST(udp, one_sender_multiple_receivers) {
    DatagramBlockingQueue queue1;
    DatagramBlockingQueue queue2;
    DatagramBlockingQueue queue3;

    Address tx_addr = make_address(0);

    Address rx1_addr = make_address(1);
    Address rx2_addr = make_address(2);
    Address rx3_addr = make_address(3);

    Transceiver tx;
    CHECK(tx.add_udp_sender(tx_addr));

    Transceiver rx1;
    CHECK(rx1.add_udp_receiver(rx1_addr, queue1));

    Transceiver rx23;
    CHECK(rx23.add_udp_receiver(rx2_addr, queue2));
    CHECK(rx23.add_udp_receiver(rx3_addr, queue3));

    tx.start();

    rx1.start();
    rx23.start();

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            send_datagram(tx, tx_addr, rx1_addr, p, 11);
            send_datagram(tx, tx_addr, rx2_addr, p, 22);
            send_datagram(tx, tx_addr, rx3_addr, p, 33);
        }
        for (int p = 0; p < NumPackets; p++) {
            wait_datagram(queue1, tx_addr, rx1_addr, p, 11);
            wait_datagram(queue2, tx_addr, rx2_addr, p, 22);
            wait_datagram(queue3, tx_addr, rx3_addr, p, 33);
        }
    }

    tx.stop();
    tx.join();

    rx1.stop();
    rx1.join();

    rx23.stop();
    rx23.join();
}

TEST(udp, multiple_senders_one_receiver) {
    DatagramBlockingQueue queue;

    Address tx1_addr = make_address(1);
    Address tx2_addr = make_address(2);
    Address tx3_addr = make_address(3);

    Address rx_addr = make_address(4);

    Transceiver tx1;
    CHECK(tx1.add_udp_sender(tx1_addr));

    Transceiver tx23;
    CHECK(tx23.add_udp_sender(tx2_addr));
    CHECK(tx23.add_udp_sender(tx3_addr));

    Transceiver rx;
    CHECK(rx.add_udp_receiver(rx_addr, queue));

    tx1.start();
    tx23.start();

    rx.start();

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            send_datagram(tx1, tx1_addr, rx_addr, p, 11);
        }
        for (int p = 0; p < NumPackets; p++) {
            wait_datagram(queue, tx1_addr, rx_addr, p, 11);
        }
        for (int p = 0; p < NumPackets; p++) {
            send_datagram(tx23, tx2_addr, rx_addr, p, 22);
        }
        for (int p = 0; p < NumPackets; p++) {
            wait_datagram(queue, tx2_addr, rx_addr, p, 22);
        }
        for (int p = 0; p < NumPackets; p++) {
            send_datagram(tx23, tx3_addr, rx_addr, p, 33);
        }
        for (int p = 0; p < NumPackets; p++) {
            wait_datagram(queue, tx3_addr, rx_addr, p, 33);
        }
    }

    tx1.stop();
    tx1.join();

    tx23.stop();
    tx23.join();

    rx.stop();
    rx.join();
}

TEST(udp, empty_sender_address) {
    DatagramBlockingQueue queue;

    Address tx_addr;
    Address rx_addr = make_address(1);

    Transceiver tx;
    CHECK(tx.add_udp_sender(tx_addr));

    Transceiver rx;
    CHECK(rx.add_udp_receiver(rx_addr, queue));

    tx.start();
    rx.start();

    for (int i = 0; i < NumIterations; i++) {
        for (int p = 0; p < NumPackets; p++) {
            send_datagram(tx, tx_addr, rx_addr, p, 99);
        }
        for (int p = 0; p < NumPackets; p++) {
            CHECK(queue.read());
        }
    }

    tx.stop();
    tx.join();

    rx.stop();
    rx.join();
}

} // namespace test
} // namespace roc
