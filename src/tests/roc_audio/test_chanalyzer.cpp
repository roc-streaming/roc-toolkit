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

const packet::channel_t CH_0 = 0, CH_1 = 1;

const packet::channel_mask_t TEST_CHANNELS = (1 << CH_0) | (1 << CH_1);

const size_t TEST_N_CHANNELS = 2;
const size_t TEST_N_PACKETS = 100;
const size_t TEST_N_ITERS = 10;

} // namespace

TEST_GROUP(chanalyzer) {
    TestPacketReader<TEST_N_PACKETS> reader;

    core::ScopedPtr<Chanalyzer> chanalyzer;

    void setup() {
        chanalyzer.reset(new Chanalyzer(reader, TEST_CHANNELS));
    }
};

TEST(chanalyzer, read_one_packet) {
    reader.add();

    reader.expect_returned(0, chanalyzer->read(CH_0));

    LONGS_EQUAL(1, reader.num_returned());
}

TEST(chanalyzer, read_two_packets) {
    reader.add();

    reader.expect_returned(0, chanalyzer->read(CH_0));
    reader.expect_returned(0, chanalyzer->read(CH_1));

    LONGS_EQUAL(1, reader.num_returned());
}

TEST(chanalyzer, read_two_packets_multiple_times) {
    for (size_t n = 0; n < TEST_N_PACKETS; n++) {
        reader.add();
    }

    for (size_t i = 0; i < TEST_N_ITERS; i++) {
        reader.rewind();

        for (size_t n = 0; n < TEST_N_PACKETS; n++) {
            LONGS_EQUAL(n, reader.num_returned());

            reader.expect_returned(n, chanalyzer->read(CH_0));
            reader.expect_returned(n, chanalyzer->read(CH_1));
        }

        LONGS_EQUAL(TEST_N_PACKETS, reader.num_returned());
    }
}

TEST(chanalyzer, read_multiple_packets) {
    enum { n_reads = 10 };

    for (size_t n = 0; n < TEST_N_PACKETS; n++) {
        reader.add();
    }

    for (size_t i = 0; i < TEST_N_ITERS; i++) {
        reader.rewind();

        size_t pos = 0;

        for (size_t n = 0; n < TEST_N_PACKETS / n_reads; n++) {
            for (packet::channel_t ch = 0; ch < TEST_N_CHANNELS; ch++) {
                for (size_t p = 0; p < n_reads; p++) {
                    reader.expect_returned(pos + p, chanalyzer->read(ch));
                }
            }
            pos += n_reads;
            LONGS_EQUAL(pos, reader.num_returned());
        }
    }
}

TEST(chanalyzer, read_null) {
    reader.add();

    reader.expect_returned(0, chanalyzer->read(CH_0));
    reader.expect_returned(0, chanalyzer->read(CH_1));

    CHECK(chanalyzer->read(CH_0) == packet::IAudioPacketPtr());
    CHECK(chanalyzer->read(CH_1) == packet::IAudioPacketPtr());

    reader.add();

    reader.expect_returned(1, chanalyzer->read(CH_0));
    reader.expect_returned(1, chanalyzer->read(CH_1));

    LONGS_EQUAL(2, reader.num_returned());
}

} // namespace test
} // namespace roc
