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

    SamplesPerFrame = 5,

    SilenceTimeout = SamplesPerFrame * 4,
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

    WatchdogConfig make_config(packet::timestamp_t silence_timeout,
                               packet::timestamp_t drops_timeout) {
        WatchdogConfig config(0);
        config.silence_timeout = silence_timeout;
        config.drops_timeout = drops_timeout;
        config.drop_detection_window = DropsWindow;
        return config;
    }

    void check_read(IReader & reader, const bool is_read, const size_t fsz,
                    const unsigned frame_flags) {
        core::Slice<sample_t> buf = new_buffer(fsz);
        memset(buf.data(), 0xff, buf.size() * sizeof(sample_t));

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

    void check_n_reads(IReader & reader, const bool is_read, const size_t fsz,
                       const size_t it_num, const unsigned frame_flags) {
        for (size_t n = 0; n < it_num; n++) {
            check_read(reader, is_read, fsz, frame_flags);
        }
    }

};

TEST(watchdog, silence_timeout_no_frames) {
    Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                      allocator);
    CHECK(watchdog.valid());

    CHECK(watchdog.update());
}

TEST(watchdog, silence_timeout_blank_frames) {
    Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                      allocator);
    CHECK(watchdog.valid());

    for (packet::timestamp_t n = 0; n < SilenceTimeout / SamplesPerFrame; n++) {
        CHECK(watchdog.update());
        check_read(watchdog, true, SamplesPerFrame, Frame::FlagBlank);
    }

    CHECK(!watchdog.update());
    check_read(watchdog, false, SamplesPerFrame, 0);
}

TEST(watchdog, silence_timeout_blank_and_non_blank_frames) {
    CHECK(SilenceTimeout % SamplesPerFrame == 0);

    Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                      allocator);
    CHECK(watchdog.valid());

    for (unsigned int i = 0; i < 2; i++) {
        for (packet::timestamp_t n = 0; n < (SilenceTimeout / SamplesPerFrame)-1; n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame, Frame::FlagBlank);
        }

        CHECK(watchdog.update());
        check_read(watchdog, true, SamplesPerFrame, 0);
    }
}

TEST(watchdog, silence_timeout_disabled) {
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        for (packet::timestamp_t n = 0; n < SilenceTimeout / SamplesPerFrame; n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame, Frame::FlagBlank);
        }

        CHECK(!watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(0, DropsTimeout), allocator);
        CHECK(watchdog.valid());

        for (packet::timestamp_t n = 0; n < SilenceTimeout / SamplesPerFrame; n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame, Frame::FlagBlank);
        }

        CHECK(watchdog.update());
    }
}

TEST(watchdog, drops_timeout_equal_frame_sizes) {
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        check_n_reads(watchdog, true, DropsWindow, DropWindowsPerTimeout - 1,
                      Frame::FlagIncomplete | Frame::FlagDrops);

        check_read(watchdog, true, DropsWindow, 0);
        CHECK(watchdog.update());
        check_read(watchdog, true, DropsWindow, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, DropsWindow, 0);
        check_n_reads(watchdog, true, DropsWindow, DropWindowsPerTimeout - 2,
                      Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow, 0);

        CHECK(watchdog.update());
        check_n_reads(watchdog, true, DropsWindow, DropWindowsPerTimeout, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, DropsWindow, 0);
        check_n_reads(watchdog, true, DropsWindow, DropWindowsPerTimeout - 1,
                      Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.update());

        check_read(watchdog, true, DropsWindow, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        check_n_reads(watchdog, true, DropsWindow, DropWindowsPerTimeout - 1,
                      Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow, Frame::FlagDrops);

        CHECK(!watchdog.update());
        check_read(watchdog, false, DropsWindow, 0);
    }
}

TEST(watchdog, drops_timeout_mixed_frame_sizes) {
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, DropsWindow * (DropWindowsPerTimeout - 1),
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow / 2, 0);
        check_read(watchdog, true, DropsWindow - DropsWindow / 2, 0);

        CHECK(watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, DropsWindow * (DropWindowsPerTimeout - 1),
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow / 2,
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow - DropsWindow / 2, 0);

        CHECK(!watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, DropsWindow * (DropWindowsPerTimeout - 1),
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow / 2, 0);
        check_read(watchdog, true, DropsWindow - DropsWindow / 2,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.update());
    }
}

TEST(watchdog, drops_timeout_constant_drops) {
    Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                      allocator);
    CHECK(watchdog.valid());

    for (packet::timestamp_t n = 0; n < DropWindowsPerTimeout; n++) {
        CHECK(watchdog.update());
        check_read(watchdog, true, DropsWindow / 2,
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow - DropsWindow / 2, 0);
    }

    CHECK(!watchdog.update());
}

TEST(watchdog, drops_timeout_frame_overlaps_with_drop_window) {
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        CHECK(watchdog.update());

        check_read(watchdog, true, DropsWindow, Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow, 0);
        check_read(watchdog, true, DropsTimeout - DropsWindow,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        CHECK(watchdog.update());

        check_read(watchdog, true, DropsWindow + 1, Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, DropsWindow - 1, 0);
        check_read(watchdog, true, DropsTimeout - DropsWindow,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        CHECK(watchdog.update());

        check_read(watchdog, true, DropsTimeout - DropsWindow, 0);
        check_read(watchdog, true, DropsWindow + 1,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.update());

        check_read(watchdog, true, DropsWindow - 1, 0);
        check_read(watchdog, true, DropsTimeout - DropsWindow, 0);

        CHECK(watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        CHECK(watchdog.update());

        check_read(watchdog, true, DropsTimeout - DropsWindow, 0);
        check_read(watchdog, true, DropsWindow + 1,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.update());

        check_read(watchdog, true, DropsWindow - 1, 0);
        check_read(watchdog, true, DropsTimeout - DropsWindow,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.update());
    }
}

TEST(watchdog, drops_timeout_disabled) {
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, DropsTimeout),
                          allocator);
        CHECK(watchdog.valid());

        for (packet::timestamp_t n = 0; n < DropsTimeout / SamplesPerFrame; n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame,
                       Frame::FlagIncomplete | Frame::FlagDrops);
        }

        CHECK(!watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(SilenceTimeout, 0), allocator);
        CHECK(watchdog.valid());

        for (packet::timestamp_t n = 0; n < DropsTimeout / SamplesPerFrame; n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame,
                       Frame::FlagIncomplete | Frame::FlagDrops);
        }

        CHECK(watchdog.update());
    }
}

} // namespace audio
} // namespace roc
