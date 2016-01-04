/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/delayed_writer.h"
#include "roc_audio/sample_buffer_queue.h"

#include "test_helpers.h"

namespace roc {
namespace test {

using namespace packet;
using namespace audio;

namespace {

enum { ChMask = 0x3, NumCh = 2, BufSz = 500 };

} // namespace

TEST_GROUP(delayed_writer){};

TEST(delayed_writer, zero_latency) {
    enum { NumBufs = 20 };

    SampleBufferQueue queue;
    DelayedWriter writer(queue, ChMask, 0);

    writer.write(ISampleBufferConstSlice());

    LONGS_EQUAL(1, queue.size());
    CHECK(!queue.read());

    for (size_t ns = NumCh; ns < NumBufs * NumCh; ns += NumCh) {
        ISampleBufferPtr buff = new_buffer<BufSz>(ns);
        writer.write(*buff);

        LONGS_EQUAL(1, queue.size());
        CHECK(queue.read().container() == buff);
    }

    writer.write(ISampleBufferConstSlice());

    LONGS_EQUAL(1, queue.size());
    CHECK(!queue.read());
}

TEST(delayed_writer, non_zero_latency) {
    enum { MaxBufs = 20, Latency = 100 };

    SampleBufferQueue queue;
    DelayedWriter writer(queue, ChMask, Latency);

    ISampleBufferPtr bufs[MaxBufs] = {};

    size_t n_bufs = 0, n_pending = 0;

    for (size_t ns = NumCh; n_pending < Latency; ns += NumCh) {
        LONGS_EQUAL(0, queue.size());

        bufs[n_bufs] = new_buffer<BufSz>(ns);
        writer.write(*bufs[n_bufs]);

        n_pending += ns / NumCh;
        n_bufs++;
    }

    LONGS_EQUAL(n_bufs, queue.size());

    for (size_t n = 0; n < n_bufs; n++) {
        CHECK(queue.read().container() == bufs[n]);
    }

    for (size_t ns = NumCh; ns < MaxBufs * NumCh; ns += NumCh) {
        ISampleBufferPtr buff = new_buffer<BufSz>(ns);
        writer.write(*buff);

        LONGS_EQUAL(1, queue.size());
        CHECK(queue.read().container() == buff);
    }

    writer.write(ISampleBufferConstSlice());

    LONGS_EQUAL(1, queue.size());
    CHECK(!queue.read());
}

TEST(delayed_writer, non_zero_latency_eof) {
    enum { MaxBufs = 20, Latency = 100 };

    SampleBufferQueue queue;
    DelayedWriter writer(queue, ChMask, Latency);

    ISampleBufferPtr bufs[MaxBufs] = {};

    size_t n_bufs = 0, n_pending = 0;

    for (size_t ns = NumCh; n_pending < Latency / 2; ns += NumCh) {
        LONGS_EQUAL(0, queue.size());

        bufs[n_bufs] = new_buffer<BufSz>(ns);
        writer.write(*bufs[n_bufs]);

        n_pending += ns / NumCh;
        n_bufs++;
    }

    writer.write(ISampleBufferConstSlice());
    n_bufs++;

    LONGS_EQUAL(n_bufs, queue.size());

    for (size_t n = 0; n < n_bufs; n++) {
        CHECK(queue.read().container() == bufs[n]);
    }
}

} // namespace test
} // namespace roc
