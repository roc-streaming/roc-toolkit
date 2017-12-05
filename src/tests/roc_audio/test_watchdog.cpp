/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/watchdog.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"

namespace roc {
namespace audio {

namespace {

enum {
    MaxBufSize = 5000,

    SamplesPerPacket = 200,
    NumCh = 2,

    Timeout = 20,

    SkipWindowSz = 5,
    SkipBatchSz = Timeout / 2
};

core::HeapAllocator allocator;
core::BufferPool<sample_t> sample_buffer_pool(allocator, MaxBufSize, 1);

class NopFrameReader : public IReader, public core::NonCopyable<> {
public:
    void read(Frame& frame) {
        memset(frame.samples().data(), 0, frame.samples().size() * sizeof(sample_t));
    }
};

NopFrameReader nop_reader;

} // namespace

TEST_GROUP(watchdog) {
    Frame new_frame(size_t sz) {
        core::Slice<sample_t> samples =
            new (sample_buffer_pool) core::Buffer<sample_t>(sample_buffer_pool);
        samples.resize(sz * NumCh);
        Frame frame(samples);
        return frame;
    }

    void check_read(unsigned frame_flags, IReader& reader, bool is_read) {
        Frame frame = new_frame(SamplesPerPacket);
        frame.add_flags(frame_flags);

        reader.read(frame);

        if (is_read) {
            for (size_t n = 0; n < frame.samples().size(); ++n) {
                DOUBLES_EQUAL(0.0f, frame.samples().data()[n], 0);
            }
        }
    }

    void check_nth_read(unsigned frame_flags, size_t sz, IReader& reader, bool is_read) {
        for (size_t n = 0; n < sz; n++) {
            check_read(frame_flags, reader, is_read);
        }
    }
};

TEST(watchdog, read) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    check_read(0, watchdog, true);

    CHECK(watchdog.update(Timeout / 4));
    check_read(0, watchdog, true);
}

TEST(watchdog, read_timeout) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    CHECK(watchdog.update(0));
    check_read(0, watchdog, true);

    CHECK(!watchdog.update(Timeout + 1));
    check_read(0, watchdog, false);
}

TEST(watchdog, read_empty_frame) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    for (packet::timestamp_t n = 0; n < Timeout; n++) {
        CHECK(watchdog.update(n));
        check_read(Frame::FlagEmpty, watchdog, true);
    }

    CHECK(!watchdog.update(Timeout + 1));
    check_read(0, watchdog, false);
}

TEST(watchdog, update_each_frame_has_skip_flag) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    for (size_t n = 0; n < SkipWindowSz - 1; n++) {
        check_read(Frame::FlagSkip, watchdog, true);
    }

    CHECK(watchdog.update(Timeout / 2));
    check_read(Frame::FlagSkip, watchdog, true);

    CHECK(!watchdog.update(Timeout));
    check_read(0, watchdog, false);
}

TEST(watchdog, update_frame_has_skip_flag) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    check_read(0, watchdog, true);

    CHECK(watchdog.update(Timeout / 2));
    check_read(Frame::FlagSkip, watchdog, true);

    CHECK(watchdog.update(Timeout / 2 + Timeout / 4));
    check_read(0, watchdog, true);

    CHECK(!watchdog.update(Timeout + Timeout / 2));
    check_read(0, watchdog, false);
}

TEST(watchdog, update_nth_frame_has_skip_flag_begin) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    for (packet::timestamp_t bs = 0; bs < SkipBatchSz; bs++) {
        check_nth_read(Frame::FlagSkip, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }

    check_nth_read(0, SkipWindowSz, watchdog, true);

    CHECK(watchdog.update(Timeout));

    check_nth_read(0, SkipWindowSz, watchdog, true);

    CHECK(watchdog.update(Timeout + Timeout / 2));
}

TEST(watchdog, update_nth_frame_has_skip_flag_middle) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    for (packet::timestamp_t bs = 0; bs < SkipBatchSz; bs++) {
        check_nth_read(0, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }

    for (packet::timestamp_t bs = SkipBatchSz; bs < Timeout; bs++) {
        check_nth_read(Frame::FlagSkip, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }

    for (packet::timestamp_t bs = Timeout; bs < Timeout + SkipBatchSz; bs++) {
        check_nth_read(0, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }
}

TEST(watchdog, update_nth_frame_has_skip_flag_end) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    for (packet::timestamp_t bs = 0; bs < SkipBatchSz; bs++) {
        check_nth_read(0, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }

    check_nth_read(Frame::FlagSkip, SkipWindowSz, watchdog, true);

    CHECK(watchdog.update(Timeout));

    check_nth_read(0, SkipWindowSz, watchdog, true);

    CHECK(watchdog.update(Timeout + Timeout / 2));
}

TEST(watchdog, update_nth_frame_has_skip_flag) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    for (packet::timestamp_t bs = 0; bs < Timeout - 1; bs++) {
        check_nth_read(Frame::FlagSkip, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }

    CHECK(!watchdog.update(Timeout));
    check_read(0, watchdog, false);
}

TEST(watchdog, update_nth_frame_has_skip_flag_after_timeout) {
    Watchdog watchdog(nop_reader, Timeout, SkipWindowSz);

    for (packet::timestamp_t bs = 0; bs < Timeout; bs++) {
        check_nth_read(0, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }

    for (packet::timestamp_t bs = Timeout; bs < Timeout + SkipBatchSz; bs++) {
        check_nth_read(Frame::FlagSkip, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }

    for (packet::timestamp_t bs = Timeout + SkipBatchSz; bs < (Timeout * 2) - 1; bs++) {
        check_nth_read(Frame::FlagSkip, SkipWindowSz, watchdog, true);
        CHECK(watchdog.update(bs));
    }

    CHECK(!watchdog.update(Timeout * 2));
    check_read(0, watchdog, false);
}

} // namespace audio
} // namespace roc
