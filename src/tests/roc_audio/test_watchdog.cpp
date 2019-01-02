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
    NumCh = 2,

    SilenceTimeout = 20,
    SamplesPerFrame = SilenceTimeout / 4,

    DropsTimeout = SamplesPerFrame * 5,
    DropsWindow = SamplesPerFrame,
    DropWindowsPerTimeout = DropsTimeout / DropsWindow
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

    void check_read(IReader& reader, const bool is_read, const size_t fsz,
                    const unsigned frame_flags) {
        core::Slice<sample_t> buf = new_buffer(fsz);
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

    void check_nth_read(IReader& reader, const bool is_read, const size_t fsz,
                        const size_t it_num, const unsigned frame_flags) {
        for (size_t n = 0; n < it_num; n++) {
            check_read(reader, is_read, fsz, frame_flags);
        }
    }

};

TEST(watchdog, first_update_timeout_exceeded) {
    Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

    CHECK(watchdog.update(SilenceTimeout * 2));
    check_read(watchdog, true, SamplesPerFrame, 0);
}

TEST(watchdog, update_timeout) {
    Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

    CHECK(watchdog.update(0));
    CHECK(!watchdog.update(SilenceTimeout));
}

TEST(watchdog, nth_update_timeout) {
    Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

    for (packet::timestamp_t n = 0; n < SilenceTimeout; n++) {
        CHECK(watchdog.update(n));
    }

    CHECK(!watchdog.update(SilenceTimeout));
}

TEST(watchdog, empty_frames_timeout) {
    Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

    for (packet::timestamp_t n = 0; n < SilenceTimeout; n++) {
        CHECK(watchdog.update(n));
        check_read(watchdog, true, SamplesPerFrame, Frame::FlagEmpty);
    }

    CHECK(!watchdog.update(SilenceTimeout));
    check_read(watchdog, false, SamplesPerFrame, 0);
}

TEST(watchdog, empty_frames_no_timeout) {
    CHECK(SilenceTimeout % SamplesPerFrame == 0);

    Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

    for (packet::timestamp_t ni = 0; ni < SilenceTimeout / SamplesPerFrame; ni++) {
        const packet::timestamp_t from = ni * SilenceTimeout;
        const packet::timestamp_t to = from + SilenceTimeout;

        for (packet::timestamp_t n = from; n < to; n++) {
            CHECK(watchdog.update(n));
            check_read(watchdog, true, SamplesPerFrame, Frame::FlagEmpty);
        }

        check_read(watchdog, true, SamplesPerFrame, 0);
        CHECK(watchdog.update(to));

        // Align read time to Timeout.
        check_read(watchdog, true, SamplesPerFrame, 0);
    }
}

TEST(watchdog, packet_drops_equal_frame_sizes) {
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_nth_read(watchdog, true, DropsWindow, DropWindowsPerTimeout - 1,
                       Frame::FlagPacketDrops);

        check_read(watchdog, true, DropsWindow, 0);
        CHECK(watchdog.update(SilenceTimeout));
        check_read(watchdog, true, DropsWindow, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_read(watchdog, true, DropsWindow, 0);
        check_nth_read(watchdog, true, DropsWindow, DropWindowsPerTimeout - 2,
                       Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow, 0);

        CHECK(watchdog.update(SilenceTimeout));
        check_nth_read(watchdog, true, DropsWindow, DropWindowsPerTimeout, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_read(watchdog, true, DropsWindow, 0);
        check_nth_read(watchdog, true, DropsWindow, DropWindowsPerTimeout - 1,
                       Frame::FlagPacketDrops);

        CHECK(watchdog.update(SilenceTimeout));

        check_read(watchdog, true, DropsWindow, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_nth_read(watchdog, true, DropsWindow, DropWindowsPerTimeout - 1,
                       Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow, Frame::FlagPacketDrops);

        CHECK(!watchdog.update(SilenceTimeout));
        check_read(watchdog, false, DropsWindow, 0);
    }
}

TEST(watchdog, frame_flags_empty_packet_drops) {
    Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

    check_read(watchdog, true, DropsWindow, Frame::FlagPacketDrops);
    check_read(watchdog, true, DropsWindow, Frame::FlagEmpty);

    CHECK(watchdog.update(SilenceTimeout));

    check_read(watchdog, true, DropsWindow, 0);
}

TEST(watchdog, frame_flags_full_packet_drops) {
    Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

    check_nth_read(watchdog, true, DropsWindow, DropWindowsPerTimeout,
                   Frame::FlagPacketDrops | Frame::FlagFull);

    CHECK(watchdog.update(SilenceTimeout));

    check_nth_read(watchdog, true, DropsWindow, DropWindowsPerTimeout, 0);
}

TEST(watchdog, packet_drops_mixed_frame_sizes) {
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_read(watchdog, true, DropsWindow * (DropWindowsPerTimeout - 1),
                   Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow / 2, Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow, 0);

        CHECK(!watchdog.update(SilenceTimeout));
    }
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_read(watchdog, true, DropsWindow * (DropWindowsPerTimeout - 1),
                   Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow / DropWindowsPerTimeout, 0);
        check_read(watchdog, true, (DropsWindow * 3) / DropWindowsPerTimeout,
                   Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow / DropWindowsPerTimeout, 0);

        CHECK(!watchdog.update(SilenceTimeout));
    }
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_read(watchdog, true, DropsWindow * (DropWindowsPerTimeout - 1),
                   Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow / 2, 0);
        check_read(watchdog, true, (DropsWindow * 3) / DropWindowsPerTimeout,
                   Frame::FlagPacketDrops);

        CHECK(!watchdog.update(SilenceTimeout));
    }
}

TEST(watchdog, frame_overlaps_drop_window) {
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_read(watchdog, true, DropsWindow, Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow, 0);
        check_read(watchdog, true, DropsTimeout - DropsWindow, Frame::FlagPacketDrops);

        CHECK(watchdog.update(SilenceTimeout));
    }
    {
        Watchdog watchdog(test_reader, NumCh, SilenceTimeout, DropsTimeout, DropsWindow);

        check_read(watchdog, true, DropsWindow + 1, Frame::FlagPacketDrops);
        check_read(watchdog, true, DropsWindow - 1, 0);
        check_read(watchdog, true, DropsTimeout - DropsWindow, Frame::FlagPacketDrops);

        CHECK(!watchdog.update(SilenceTimeout));
    }
}

} // namespace audio
} // namespace roc
