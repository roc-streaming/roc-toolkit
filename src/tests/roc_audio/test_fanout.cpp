/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_writer.h"

#include "roc_audio/fanout.h"
#include "roc_audio/frame_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

enum { BufSz = 100, MaxSz = 500 };

const SampleSpec sample_spec(44100,
                             PcmSubformat_Raw,
                             ChanLayout_Surround,
                             ChanOrder_Smpte,
                             ChanMask_Surround_Stereo);

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxSz * sizeof(sample_t));

FramePtr new_frame(size_t sz) {
    FramePtr frame = frame_factory.allocate_frame(sz * sizeof(sample_t));
    CHECK(frame);

    frame->set_raw(true);
    frame->set_duration(sz / sample_spec.num_channels());

    return frame;
}

void write_frame(Fanout& fanout, size_t sz, sample_t value) {
    FramePtr frame = new_frame(sz);

    for (size_t n = 0; n < sz; n++) {
        frame->raw_samples()[n] = value;
    }

    LONGS_EQUAL(status::StatusOK, fanout.write(*frame));
}

void expect_written(test::MockWriter& mock_writer, size_t sz, sample_t value) {
    for (size_t n = 0; n < sz; n++) {
        DOUBLES_EQUAL((double)value, (double)mock_writer.get(), 0.0001);
    }
}

} // namespace

TEST_GROUP(fanout) {};

TEST(fanout, no_writers) {
    Fanout fanout(sample_spec, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, fanout.init_status());

    write_frame(fanout, BufSz, 0.11f);
}

TEST(fanout, one_output) {
    test::MockWriter writer;

    Fanout fanout(sample_spec, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, fanout.init_status());

    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer));

    write_frame(fanout, BufSz, 0.11f);

    LONGS_EQUAL(BufSz, writer.num_unread());
    expect_written(writer, BufSz, 0.11f);

    LONGS_EQUAL(0, writer.num_unread());
}

TEST(fanout, two_outputs) {
    test::MockWriter writer1;
    test::MockWriter writer2;

    Fanout fanout(sample_spec, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, fanout.init_status());

    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer1));
    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer2));

    write_frame(fanout, BufSz, 0.11f);

    LONGS_EQUAL(BufSz, writer1.num_unread());
    expect_written(writer1, BufSz, 0.11f);

    LONGS_EQUAL(BufSz, writer2.num_unread());
    expect_written(writer2, BufSz, 0.11f);

    LONGS_EQUAL(0, writer1.num_unread());
    LONGS_EQUAL(0, writer2.num_unread());
}

TEST(fanout, remove_output) {
    test::MockWriter writer1;
    test::MockWriter writer2;
    test::MockWriter writer3;

    Fanout fanout(sample_spec, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, fanout.init_status());

    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer1));
    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer2));
    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer3));

    write_frame(fanout, BufSz, 0.11f);

    LONGS_EQUAL(BufSz, writer1.num_unread());
    LONGS_EQUAL(BufSz, writer2.num_unread());
    LONGS_EQUAL(BufSz, writer3.num_unread());

    fanout.remove_output(writer2);

    write_frame(fanout, BufSz, 0.22f);

    LONGS_EQUAL(BufSz * 2, writer1.num_unread());
    LONGS_EQUAL(BufSz, writer2.num_unread());
    LONGS_EQUAL(BufSz * 2, writer3.num_unread());
}

TEST(fanout, has_output) {
    test::MockWriter writer1;
    test::MockWriter writer2;

    Fanout fanout(sample_spec, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, fanout.init_status());

    CHECK(!fanout.has_output(writer1));
    CHECK(!fanout.has_output(writer2));

    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer1));
    CHECK(fanout.has_output(writer1));
    CHECK(!fanout.has_output(writer2));

    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer2));
    CHECK(fanout.has_output(writer1));
    CHECK(fanout.has_output(writer2));

    fanout.remove_output(writer1);
    CHECK(!fanout.has_output(writer1));
    CHECK(fanout.has_output(writer2));

    fanout.remove_output(writer2);
    CHECK(!fanout.has_output(writer1));
    CHECK(!fanout.has_output(writer2));
}

TEST(fanout, forward_error) {
    test::MockWriter writer1;
    test::MockWriter writer2;

    Fanout fanout(sample_spec, frame_factory, arena);
    LONGS_EQUAL(status::StatusOK, fanout.init_status());

    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer1));
    LONGS_EQUAL(status::StatusOK, fanout.add_output(writer2));

    FramePtr frame = new_frame(BufSz);
    CHECK(frame);

    writer1.set_status(status::StatusAbort);
    writer2.set_status(status::StatusOK);

    LONGS_EQUAL(status::StatusAbort, fanout.write(*frame));

    writer1.set_status(status::StatusOK);
    writer2.set_status(status::StatusAbort);

    LONGS_EQUAL(status::StatusAbort, fanout.write(*frame));

    writer1.set_status(status::StatusOK);
    writer2.set_status(status::StatusOK);

    LONGS_EQUAL(status::StatusOK, fanout.write(*frame));
}

} // namespace audio
} // namespace roc
