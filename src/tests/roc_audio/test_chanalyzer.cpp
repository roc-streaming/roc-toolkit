/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/scoped_ptr.h"

#include "roc_audio/chanalyzer.h"

#include "test_packet_reader.h"

namespace roc {
namespace test {

using namespace audio;

namespace {

enum { Ch0 = 0, Ch1 = 1, NumCh = 2, ChMask = (1 << Ch0) | (1 << Ch1) };

enum { NumPackets = 100, NumIters = 10 };

} // namespace

TEST_GROUP(chanalyzer) {
    TestPacketReader<NumPackets> reader;

    core::ScopedPtr<Chanalyzer> chanalyzer;

    void setup() {
        chanalyzer.reset(new Chanalyzer(reader, ChMask));
    }
};

TEST(chanalyzer, read_one_packet) {
    reader.add();

    reader.expect_returned(0, chanalyzer->reader(Ch0).read());

    LONGS_EQUAL(1, reader.num_returned());
}

TEST(chanalyzer, read_two_packets) {
    reader.add();

    reader.expect_returned(0, chanalyzer->reader(Ch0).read());
    reader.expect_returned(0, chanalyzer->reader(Ch1).read());

    LONGS_EQUAL(1, reader.num_returned());
}

TEST(chanalyzer, read_two_packets_multiple_times) {
    for (size_t n = 0; n < NumPackets; n++) {
        reader.add();
    }

    for (size_t i = 0; i < NumIters; i++) {
        reader.rewind();

        for (size_t n = 0; n < NumPackets; n++) {
            LONGS_EQUAL(n, reader.num_returned());

            reader.expect_returned(n, chanalyzer->reader(Ch0).read());
            reader.expect_returned(n, chanalyzer->reader(Ch1).read());
        }

        LONGS_EQUAL(NumPackets, reader.num_returned());
    }
}

TEST(chanalyzer, read_multiple_packets) {
    enum { n_reads = 10 };

    for (size_t n = 0; n < NumPackets; n++) {
        reader.add();
    }

    for (size_t i = 0; i < NumIters; i++) {
        reader.rewind();

        size_t pos = 0;

        for (size_t n = 0; n < NumPackets / n_reads; n++) {
            for (packet::channel_t ch = 0; ch < NumCh; ch++) {
                for (size_t p = 0; p < n_reads; p++) {
                    reader.expect_returned(pos + p, chanalyzer->reader(ch).read());
                }
            }
            pos += n_reads;
            LONGS_EQUAL(pos, reader.num_returned());
        }
    }
}

TEST(chanalyzer, read_null) {
    reader.add();

    reader.expect_returned(0, chanalyzer->reader(Ch0).read());
    reader.expect_returned(0, chanalyzer->reader(Ch1).read());

    CHECK(chanalyzer->reader(Ch0).read() == packet::IPacketPtr());
    CHECK(chanalyzer->reader(Ch1).read() == packet::IPacketPtr());

    reader.add();

    reader.expect_returned(1, chanalyzer->reader(Ch0).read());
    reader.expect_returned(1, chanalyzer->reader(Ch1).read());

    LONGS_EQUAL(2, reader.num_returned());
}

} // namespace test
} // namespace roc
