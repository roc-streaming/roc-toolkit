/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_writer.h"

#include "roc_audio/fanout.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

enum { BufSz = 100, MaxSz = 500 };

core::HeapAllocator allocator;
core::BufferPool<sample_t> buffer_pool(allocator, MaxSz, true);

} // namespace

TEST_GROUP(fanout) {
    core::Slice<sample_t> new_buffer(size_t sz) {
        core::Slice<sample_t> buf =
            new (buffer_pool) core::Buffer<sample_t>(buffer_pool);
        buf.resize(sz);
        return buf;
    }

    void write_frame(Fanout& fanout, size_t sz, sample_t value) {
        core::Slice<sample_t> buf = new_buffer(sz);

        Frame frame(buf.data(), buf.size());
        for (size_t n = 0; n < sz; n++) {
            frame.data()[n] = value;
        }

        fanout.write(frame);
    }

    void expect_written(test::MockWriter& mock_writer, size_t sz, sample_t value) {
        for (size_t n = 0; n < sz; n++) {
            DOUBLES_EQUAL((double)value, (double)mock_writer.get(), 0.0001);
        }
    }
};

TEST(fanout, no_writers) {
    Fanout fanout;

    write_frame(fanout, BufSz, 0.11f);
}

TEST(fanout, one_output) {
    test::MockWriter writer;

    Fanout fanout;
    fanout.add_output(writer);

    write_frame(fanout, BufSz, 0.11f);

    CHECK(writer.num_unread() == BufSz);
    expect_written(writer, BufSz, 0.11f);

    CHECK(writer.num_unread() == 0);
}

TEST(fanout, two_outputs) {
    test::MockWriter writer1;
    test::MockWriter writer2;

    Fanout fanout;
    fanout.add_output(writer1);
    fanout.add_output(writer2);

    write_frame(fanout, BufSz, 0.11f);

    CHECK(writer1.num_unread() == BufSz);
    expect_written(writer1, BufSz, 0.11f);

    CHECK(writer2.num_unread() == BufSz);
    expect_written(writer2, BufSz, 0.11f);

    CHECK(writer1.num_unread() == 0);
    CHECK(writer2.num_unread() == 0);
}

TEST(fanout, remove_output) {
    test::MockWriter writer1;
    test::MockWriter writer2;
    test::MockWriter writer3;

    Fanout fanout;
    fanout.add_output(writer1);
    fanout.add_output(writer2);
    fanout.add_output(writer3);

    write_frame(fanout, BufSz, 0.11f);

    CHECK(writer1.num_unread() == BufSz);
    CHECK(writer2.num_unread() == BufSz);
    CHECK(writer3.num_unread() == BufSz);

    fanout.remove_output(writer2);

    write_frame(fanout, BufSz, 0.22f);

    CHECK(writer1.num_unread() == BufSz * 2);
    CHECK(writer2.num_unread() == BufSz);
    CHECK(writer3.num_unread() == BufSz * 2);
}

TEST(fanout, has_output) {
    test::MockWriter writer;
    Fanout fanout;

    CHECK(!fanout.has_output(writer));

    fanout.add_output(writer);
    CHECK(fanout.has_output(writer));

    fanout.remove_output(writer);
    CHECK(!fanout.has_output(writer));
}

} // namespace audio
} // namespace roc
