/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/watchdog.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/slice.h"

namespace roc {
namespace audio {

namespace {

enum {
    MaxBufSize = 500,

    NumCh = 2,
    ChMask = 0x3,
    SamplesPerFrame = 5,

    SampleRate = 1000,

    NoPlaybackTimeout = SamplesPerFrame * 4,
    BrokenPlaybackTimeout = SamplesPerFrame * 5,
    BreakageWindow = SamplesPerFrame,
    BreakageWindowsPerTimeout = BrokenPlaybackTimeout / BreakageWindow
};
const SampleSpec SampleSpecs(SampleRate, ChanLayout_Surround, ChanOrder_Smpte, ChMask);

core::HeapArena arena;
core::BufferFactory<sample_t> sample_buffer_factory(arena, MaxBufSize);

class TestFrameReader : public IFrameReader, public core::NonCopyable<> {
public:
    TestFrameReader()
        : flags_(0) {
    }

    void set_flags(unsigned flags) {
        flags_ = flags;
    }

    bool read(Frame& frame) {
        if (flags_) {
            frame.set_flags(flags_);
        }
        for (size_t n = 0; n < frame.num_samples(); n++) {
            frame.samples()[n] = 42;
        }
        return true;
    }

private:
    unsigned flags_;
};

} // namespace

TEST_GROUP(watchdog) {
    TestFrameReader test_reader;

    core::Slice<sample_t> new_buffer(size_t sz) {
        core::Slice<sample_t> buf = sample_buffer_factory.new_buffer();
        buf.reslice(0, sz * NumCh);
        return buf;
    }

    WatchdogConfig make_config(packet::stream_timestamp_t no_playback_timeout,
                               packet::stream_timestamp_t broken_playback_timeout) {
        WatchdogConfig config;
        config.no_playback_timeout = no_playback_timeout * core::Second / SampleRate;
        config.choppy_playback_timeout =
            broken_playback_timeout * core::Second / SampleRate;
        config.choppy_playback_window = BreakageWindow * core::Second / SampleRate;
        return config;
    }

    void check_read(IFrameReader & reader, bool is_read, size_t fsz,
                    unsigned frame_flags) {
        core::Slice<sample_t> buf = new_buffer(fsz);
        memset(buf.data(), 0xff, buf.size() * sizeof(sample_t));

        test_reader.set_flags(frame_flags);

        Frame frame(buf.data(), buf.size());
        CHECK(reader.read(frame));

        if (is_read) {
            for (size_t n = 0; n < frame.num_samples(); n++) {
                DOUBLES_EQUAL(42.0, (double)frame.samples()[n], 0);
            }
        } else {
            for (size_t n = 0; n < frame.num_samples(); n++) {
                DOUBLES_EQUAL(0.0, (double)frame.samples()[n], 0);
            }
        }
    }

    void check_n_reads(IFrameReader & reader, bool is_read, size_t fsz, size_t it_num,
                       unsigned frame_flags) {
        for (size_t n = 0; n < it_num; n++) {
            check_read(reader, is_read, fsz, frame_flags);
        }
    }
};

TEST(watchdog, no_playback_timeout_no_frames) {
    Watchdog watchdog(test_reader, SampleSpecs,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
    CHECK(watchdog.is_valid());
    CHECK(watchdog.is_alive());
}

TEST(watchdog, no_playback_timeout_blank_frames) {
    Watchdog watchdog(test_reader, SampleSpecs,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
    CHECK(watchdog.is_valid());

    for (packet::stream_timestamp_t n = 0; n < NoPlaybackTimeout / SamplesPerFrame; n++) {
        CHECK(watchdog.is_alive());
        check_read(watchdog, true, SamplesPerFrame, 0);
    }

    CHECK(!watchdog.is_alive());
    check_read(watchdog, false, SamplesPerFrame, Frame::FlagNonblank);

    CHECK(!watchdog.is_alive());
}

TEST(watchdog, no_playback_timeout_blank_and_non_blank_frames) {
    CHECK(NoPlaybackTimeout % SamplesPerFrame == 0);

    Watchdog watchdog(test_reader, SampleSpecs,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
    CHECK(watchdog.is_valid());

    for (unsigned int i = 0; i < 2; i++) {
        for (packet::stream_timestamp_t n = 0;
             n < (NoPlaybackTimeout / SamplesPerFrame) - 1; n++) {
            CHECK(watchdog.is_alive());
            check_read(watchdog, true, SamplesPerFrame, 0);
        }

        CHECK(watchdog.is_alive());
        check_read(watchdog, true, SamplesPerFrame, Frame::FlagNonblank);
    }

    CHECK(watchdog.is_alive());
}

TEST(watchdog, no_playback_timeout_disabled) {
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        for (packet::stream_timestamp_t n = 0; n < NoPlaybackTimeout / SamplesPerFrame;
             n++) {
            CHECK(watchdog.is_alive());
            check_read(watchdog, true, SamplesPerFrame, 0);
        }

        CHECK(!watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs, make_config(0, BrokenPlaybackTimeout),
                          arena);
        CHECK(watchdog.is_valid());

        for (packet::stream_timestamp_t n = 0; n < NoPlaybackTimeout / SamplesPerFrame;
             n++) {
            CHECK(watchdog.is_alive());
            check_read(watchdog, true, SamplesPerFrame, 0);
        }

        CHECK(watchdog.is_alive());
    }
}

TEST(watchdog, broken_playback_timeout_equal_frame_sizes) {
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);

        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        CHECK(watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 2,
                      Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        CHECK(watchdog.is_alive());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout,
                      Frame::FlagNonblank);
        CHECK(watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        CHECK(watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow,
                   Frame::FlagNonblank | Frame::FlagIncomplete);
        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        CHECK(watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow,
                   Frame::FlagNonblank | Frame::FlagDrops);
        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        CHECK(watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_n_reads(watchdog, true, BreakageWindow, BreakageWindowsPerTimeout - 1,
                      Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        CHECK(!watchdog.is_alive());

        check_read(watchdog, false, BreakageWindow, Frame::FlagNonblank);
        CHECK(!watchdog.is_alive());
    }
}

TEST(watchdog, broken_playback_timeout_mixed_frame_sizes) {
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_read(watchdog, true, BreakageWindow * (BreakageWindowsPerTimeout - 1),
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow / 2, Frame::FlagNonblank);
        check_read(watchdog, true, BreakageWindow - BreakageWindow / 2,
                   Frame::FlagNonblank);

        CHECK(watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_read(watchdog, true, BreakageWindow * (BreakageWindowsPerTimeout - 1),
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow / 2,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow - BreakageWindow / 2,
                   Frame::FlagNonblank);

        CHECK(!watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        check_read(watchdog, true, BreakageWindow * (BreakageWindowsPerTimeout - 1),
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow / 2, Frame::FlagNonblank);
        check_read(watchdog, true, BreakageWindow - BreakageWindow / 2,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.is_alive());
    }
}

TEST(watchdog, broken_playback_timeout_constant_drops) {
    Watchdog watchdog(test_reader, SampleSpecs,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
    CHECK(watchdog.is_valid());

    for (packet::stream_timestamp_t n = 0; n < BreakageWindowsPerTimeout; n++) {
        CHECK(watchdog.is_alive());
        check_read(watchdog, true, BreakageWindow / 2,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow - BreakageWindow / 2,
                   Frame::FlagNonblank);
    }

    CHECK(!watchdog.is_alive());
}

TEST(watchdog, broken_playback_timeout_frame_overlaps_with_breakage_window) {
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BreakageWindow,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow, Frame::FlagNonblank);
        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BreakageWindow + 1,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        check_read(watchdog, true, BreakageWindow - 1, Frame::FlagNonblank);
        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagNonblank);
        check_read(watchdog, true, BreakageWindow + 1,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BreakageWindow - 1, Frame::FlagNonblank);
        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagNonblank);

        CHECK(watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagNonblank);
        check_read(watchdog, true, BreakageWindow + 1,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(watchdog.is_alive());

        check_read(watchdog, true, BreakageWindow - 1, Frame::FlagNonblank);
        check_read(watchdog, true, BrokenPlaybackTimeout - BreakageWindow,
                   Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);

        CHECK(!watchdog.is_alive());
    }
}

TEST(watchdog, broken_playback_timeout_disabled) {
    {
        Watchdog watchdog(test_reader, SampleSpecs,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout), arena);
        CHECK(watchdog.is_valid());

        for (packet::stream_timestamp_t n = 0;
             n < BrokenPlaybackTimeout / SamplesPerFrame; n++) {
            CHECK(watchdog.is_alive());
            check_read(watchdog, true, SamplesPerFrame,
                       Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        }

        CHECK(!watchdog.is_alive());
    }
    {
        Watchdog watchdog(test_reader, SampleSpecs, make_config(NoPlaybackTimeout, 0),
                          arena);
        CHECK(watchdog.is_valid());

        for (packet::stream_timestamp_t n = 0;
             n < BrokenPlaybackTimeout / SamplesPerFrame; n++) {
            CHECK(watchdog.is_alive());
            check_read(watchdog, true, SamplesPerFrame,
                       Frame::FlagNonblank | Frame::FlagIncomplete | Frame::FlagDrops);
        }

        CHECK(watchdog.is_alive());
    }
}

} // namespace audio
} // namespace roc
