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
    MaxBufSize = 500,

    NumCh = 2,
    SamplesPerFrame = 5,

    SampleRate = 1000,

    NoPlaybackTimeout = SamplesPerFrame * 4,
    BrokenPlaybackTimeout = SamplesPerFrame * 5,
    BreakageWindow = SamplesPerFrame,
    BreakageWindowsPerTimeout = BrokenPlaybackTimeout / BreakageWindow
};

core::HeapAllocator allocator;
core::BufferPool<sample_t> sample_buffer_pool(allocator, MaxBufSize, true);

class TestFrameReader : public IReader, public core::NonCopyable<> {
public:
    TestFrameReader()
        : flags_(0) {
    }

    void set_flags(unsigned flags) {
        flags_ = flags;
    }

    ssize_t read(Frame& frame) {
        if (flags_) {
            frame.set_flags(flags_);
        }
        for (size_t n = 0; n < frame.size(); n++) {
            frame.data()[n] = 42;
        }
        return frame.size();
    }

private:
    unsigned flags_;
};

} // namespace

TEST_GROUP(watchdog) {
    TestFrameReader test_reader;

    core::Slice<sample_t> new_buffer(size_t sz) {
        core::Slice<sample_t> buf =
            new (sample_buffer_pool) core::Buffer<sample_t>(sample_buffer_pool);
        buf.reslice(0, sz * NumCh);
        return buf;
    }

    WatchdogConfig make_config(packet::timestamp_t no_playback_timeout,
                               packet::timestamp_t broken_playback_timeout) {
        WatchdogConfig config;
        config.no_playback_timeout = no_playback_timeout * core::Second / SampleRate;
        config.broken_playback_timeout =
            broken_playback_timeout * core::Second / SampleRate;
        config.breakage_detection_window = BreakageWindow * core::Second / SampleRate;
        return config;
    }

    void check_read(IReader & reader, bool is_read, size_t fsz, unsigned frame_flags) {
        core::Slice<sample_t> buf = new_buffer(fsz);
        memset(buf.data(), 0xff, buf.size() * sizeof(sample_t));

        test_reader.set_flags(frame_flags);

        Frame frame(buf.data(), buf.size());
        ssize_t ret_val = reader.read(frame);
        CHECK(ret_val >= 0);

        if (is_read) {
            for (size_t n = 0; n < frame.size(); n++) {
                DOUBLES_EQUAL(42.0, (double)frame.data()[n], 0);
            }
        } else {
            for (size_t n = 0; n < frame.size(); n++) {
                DOUBLES_EQUAL(0.0, (double)frame.data()[n], 0);
            }
        }
    }

    void check_n_reads(IReader & reader, bool is_read, size_t fsz, size_t it_num,
                       unsigned frame_flags) {
        for (size_t n = 0; n < it_num; n++) {
            check_read(reader, is_read, fsz, frame_flags);
        }
    }
};

TEST(watchdog, no_playback_timeout_no_frames) {
    Watchdog watchdog(test_reader, NumCh,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), SampleRate,
                      allocator);
    CHECK(watchdog.valid());

    CHECK(watchdog.update());
}

TEST(watchdog, no_playback_timeout_blank_frames) {
    Watchdog watchdog(test_reader, NumCh,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), SampleRate,
                      allocator);
    CHECK(watchdog.valid());

    for (packet::timestamp_t n = 0; n < NoPlaybackTimeout / SamplesPerFrame; n++) {
        CHECK(watchdog.update());
        check_read(watchdog, true, SamplesPerFrame, Frame::FlagBlank);
    }

    CHECK(!watchdog.update());
    check_read(watchdog, false, SamplesPerFrame, 0);
}

TEST(watchdog, no_playback_timeout_blank_and_non_blank_frames) {
    CHECK(NoPlaybackTimeout % SamplesPerFrame == 0);

    Watchdog watchdog(test_reader, NumCh,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), SampleRate,
                      allocator);
    CHECK(watchdog.valid());

    for (unsigned int i = 0; i < 2; i++) {
        for (packet::timestamp_t n = 0; n < (NoPlaybackTimeout / SamplesPerFrame) - 1;
             n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame, Frame::FlagBlank);
        }

        CHECK(watchdog.update());
        check_read(watchdog, true, SamplesPerFrame, 0);
    }
}

TEST(watchdog, no_playback_timeout_disabled) {
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        for (packet::timestamp_t n = 0; n < NoPlaybackTimeout / SamplesPerFrame; n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame, Frame::FlagBlank);
        }

        CHECK(!watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(0, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        for (packet::timestamp_t n = 0; n < NoPlaybackTimeout / SamplesPerFrame; n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame, Frame::FlagBlank);
        }

        CHECK(watchdog.update());
    }
}

TEST(watchdog, broken_playback_timeout_equal_frame_sizes) {
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagIncomplete | Frame::FlagDrops);

        check_read(watchdog, true, BreakageWindow, 0);
        CHECK(watchdog.update());
        check_read(watchdog, true, BreakageWindow, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, BreakageWindow, 0);
        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 2,
                      Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow, 0);

        CHECK(watchdog.update());
        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, BreakageWindow, 0);
        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.update());

        check_read(watchdog, true, BreakageWindow, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow, Frame::FlagIncomplete);

        CHECK(watchdog.update());
        check_read(watchdog, true, BreakageWindow, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow, Frame::FlagDrops);

        CHECK(watchdog.update());
        check_read(watchdog, true, BreakageWindow, 0);
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.update());
        check_read(watchdog, false, BreakageWindow, 0);
    }
}

TEST(watchdog, broken_playback_timeout_mixed_frame_sizes) {
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, BreakageWindow * (BreakageWindowsPerTimeout - 1),
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow / 2, 0);
        check_read(watchdog, true, BreakageWindow - BreakageWindow / 2, 0);

        CHECK(watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, BreakageWindow * (BreakageWindowsPerTimeout - 1),
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow / 2,
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow - BreakageWindow / 2, 0);

        CHECK(!watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        check_read(watchdog, true, BreakageWindow * (BreakageWindowsPerTimeout - 1),
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow / 2, 0);
        check_read(watchdog, true, BreakageWindow - BreakageWindow / 2,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.update());
    }
}

TEST(watchdog, broken_playback_timeout_constant_drops) {
    Watchdog watchdog(test_reader, NumCh,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), SampleRate,
                      allocator);
    CHECK(watchdog.valid());

    for (packet::timestamp_t n = 0; n < BreakageWindowsPerTimeout; n++) {
        CHECK(watchdog.update());
        check_read(watchdog, true, BreakageWindow / 2,
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow - BreakageWindow / 2, 0);
    }

    CHECK(!watchdog.update());
}

TEST(watchdog, broken_playback_timeout_frame_overlaps_with_breakage_window) {
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        CHECK(watchdog.update());

        check_read(watchdog, true, BreakageWindow,
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow, 0);
        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        CHECK(watchdog.update());

        check_read(watchdog, true, BreakageWindow + 1,
                   Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow - 1, 0);
        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        CHECK(watchdog.update());

        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow, 0);
        check_read(watchdog, true, BreakageWindow + 1,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.update());

        check_read(watchdog, true, BreakageWindow - 1, 0);
        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow, 0);

        CHECK(watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        CHECK(watchdog.update());

        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow, 0);
        check_read(watchdog, true, BreakageWindow + 1,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.update());

        check_read(watchdog, true, BreakageWindow - 1, 0);
        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.update());
    }
}

TEST(watchdog, broken_playback_timeout_disabled) {
    {
        Watchdog watchdog(test_reader, NumCh,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        for (packet::timestamp_t n = 0; n < BrokenPlaybackTimeout / SamplesPerFrame;
             n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame,
                       Frame::FlagIncomplete | Frame::FlagDrops);
        }

        CHECK(!watchdog.update());
    }
    {
        Watchdog watchdog(test_reader, NumCh, make_config(NoPlaybackTimeout, 0),
                          SampleRate, allocator);
        CHECK(watchdog.valid());

        for (packet::timestamp_t n = 0; n < BrokenPlaybackTimeout / SamplesPerFrame;
             n++) {
            CHECK(watchdog.update());
            check_read(watchdog, true, SamplesPerFrame,
                       Frame::FlagIncomplete | Frame::FlagDrops);
        }

        CHECK(watchdog.update());
    }
}

} // namespace audio
} // namespace roc
