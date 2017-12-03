/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/watchdog.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/slice.h"

namespace roc {
namespace audio {

namespace {

enum {
    MaxBufSize = 5000,
    Timeout = 20,
    SamplesPerFrame = Timeout / 10,
    NumCh = 2
};

core::HeapAllocator allocator;
core::BufferPool<sample_t> sample_buffer_pool(allocator, MaxBufSize, 1);

class TestFrameReader : public IReader, public core::NonCopyable<> {
public:
    void read(Frame& frame) {
        for (size_t n = 0; n < frame.size(); n++) {
            frame.data()[n] = 42;
        }
    }
};

} // namespace

TEST_GROUP(watchdog) {
    TestFrameReader test_reader;

    core::Slice<sample_t> new_buffer(size_t sz) {
        core::Slice<sample_t> buf =
            new (sample_buffer_pool) core::Buffer<sample_t>(sample_buffer_pool);
        buf.resize(sz * NumCh);
        return buf;
    }

    void check_read(unsigned frame_flags, IReader& reader, bool is_read) {
        core::Slice<sample_t> buf = new_buffer(SamplesPerFrame);
        Frame frame(buf.data(), buf.size());
        frame.add_flags(frame_flags);

        reader.read(frame);

        if (is_read) {
            for (size_t n = 0; n < frame.size(); n++) {
                DOUBLES_EQUAL(42.0f, frame.data()[n], 0);
            }
        } else {
            for (size_t n = 0; n < frame.size(); n++) {
                DOUBLES_EQUAL(0.0f, frame.data()[n], 0);
            }
        }
    }
};

TEST(watchdog, first_update_timeout_exceeded) {
    Watchdog watchdog(test_reader, Timeout);

    CHECK(watchdog.update(Timeout * 2));
    check_read(0, watchdog, true);
}

TEST(watchdog, update_timeout) {
    Watchdog watchdog(test_reader, Timeout);

    CHECK(watchdog.update(0));
    CHECK(!watchdog.update(Timeout));
}

TEST(watchdog, nth_update_timeout) {
    Watchdog watchdog(test_reader, Timeout);

    for (packet::timestamp_t n = 0; n < Timeout; n++) {
        CHECK(watchdog.update(n));
    }

    CHECK(!watchdog.update(Timeout));
}

TEST(watchdog, empty_frames_timeout) {
    Watchdog watchdog(test_reader, Timeout);

    for (packet::timestamp_t n = 0; n < Timeout; n++) {
        CHECK(watchdog.update(n));
        check_read(Frame::FlagEmpty, watchdog, true);
    }

    CHECK(!watchdog.update(Timeout));
    check_read(0, watchdog, false);
}

TEST(watchdog, empty_frames_no_timeout) {
    CHECK(Timeout % SamplesPerFrame == 0);

    Watchdog watchdog(test_reader, Timeout);

    for (packet::timestamp_t ni = 0; ni < Timeout / SamplesPerFrame; ni++) {
        const packet::timestamp_t from = ni * Timeout;
        const packet::timestamp_t to = from + Timeout;

        for (packet::timestamp_t n = from; n < to; n++) {
            CHECK(watchdog.update(n));
            check_read(Frame::FlagEmpty, watchdog, true);
        }

        check_read(0, watchdog, true);
        CHECK(watchdog.update(to));

        // Align read time to Timeout.
        check_read(0, watchdog, true);
    }
}

} // namespace audio
} // namespace roc
