/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/frame_factory.h"
#include "roc_audio/watchdog.h"
#include "roc_core/heap_arena.h"
#include "roc_core/slice.h"

namespace roc {
namespace audio {

namespace {

enum {
    MaxBufSize = 500,

    NumCh = 2,
    ChMask = 0x3,
    SamplesPerFrame = 6,

    SampleRate = 1000,

    NoPlaybackTimeout = SamplesPerFrame * 4,
    BrokenPlaybackTimeout = SamplesPerFrame * 5,
    BreakageWindow = SamplesPerFrame,
    BreakageWindowsPerTimeout = BrokenPlaybackTimeout / BreakageWindow
};

const sample_t magic_sample = 42;

const SampleSpec sample_spec(
    SampleRate, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte, ChMask);

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxBufSize * sizeof(sample_t));

WatchdogConfig
make_config(int no_playback_timeout, int broken_playback_timeout, int warmup_duration) {
    WatchdogConfig config;
    config.no_playback_timeout =
        no_playback_timeout >= 0 ? no_playback_timeout * core::Second / SampleRate : -1;
    config.choppy_playback_timeout = broken_playback_timeout >= 0
        ? broken_playback_timeout * core::Second / SampleRate
        : -1;
    config.choppy_playback_window = BreakageWindow * core::Second / SampleRate;
    config.warmup_duration =
        warmup_duration >= 0 ? warmup_duration * core::Second / SampleRate : -1;
    return config;
}

void expect_read(status::StatusCode expected_status,
                 Watchdog& watchdog,
                 size_t requested_duration,
                 size_t expected_duration = 0,
                 FrameReadMode mode = ModeHard) {
    if (expected_duration == 0) {
        expected_duration = requested_duration;
    }

    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(expected_status, watchdog.read(*frame, requested_duration, mode));

    if (expected_status == status::StatusOK || expected_status == status::StatusPart) {
        CHECK(frame->is_raw());
        UNSIGNED_LONGS_EQUAL(expected_duration * NumCh, frame->num_raw_samples());
        UNSIGNED_LONGS_EQUAL(expected_duration, frame->duration());

        for (size_t n = 0; n < frame->num_raw_samples(); n++) {
            DOUBLES_EQUAL(magic_sample, (double)frame->raw_samples()[n], 0);
        }
    }
}

void expect_n_reads(status::StatusCode expected_status,
                    Watchdog& watchdog,
                    size_t duration,
                    size_t read_count) {
    for (size_t n = 0; n < read_count; n++) {
        expect_read(expected_status, watchdog, duration);
    }
}

class MetaReader : public IFrameReader, public core::NonCopyable<> {
public:
    MetaReader()
        : flags_(0)
        , max_duration_(0)
        , status_(status::NoStatus)
        , last_status_(status::NoStatus)
        , last_mode_((FrameReadMode)-1) {
    }

    virtual status::StatusCode read(Frame& frame,
                                    packet::stream_timestamp_t requested_duration,
                                    FrameReadMode mode) {
        last_mode_ = mode;

        if (status_ != status::NoStatus) {
            return (last_status_ = status_);
        }

        packet::stream_timestamp_t duration = requested_duration;

        if (max_duration_ != 0) {
            duration = std::min(duration, max_duration_);
        }

        CHECK(frame_factory.reallocate_frame(
            frame, sample_spec.stream_timestamp_2_bytes(duration)));

        frame.set_flags(flags_);
        frame.set_raw(true);
        frame.set_duration(frame.num_raw_samples() / NumCh);

        for (size_t n = 0; n < frame.num_raw_samples(); n++) {
            frame.raw_samples()[n] = magic_sample;
        }

        return (last_status_ = (duration == requested_duration ? status::StatusOK
                                                               : status::StatusPart));
    }

    void set_flags(unsigned flags) {
        flags_ = flags;
    }

    void set_limit(packet::stream_timestamp_t max_duration) {
        max_duration_ = max_duration;
    }

    void set_status(status::StatusCode status) {
        status_ = status;
    }

    status::StatusCode last_status() const {
        return last_status_;
    }

    FrameReadMode last_mode() const {
        return last_mode_;
    }

private:
    unsigned flags_;
    packet::stream_timestamp_t max_duration_;
    status::StatusCode status_;
    status::StatusCode last_status_;
    FrameReadMode last_mode_;
};

} // namespace

TEST_GROUP(watchdog) {};

TEST(watchdog, no_playback_timeout_blank_frames) {
    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1), arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    for (packet::stream_timestamp_t n = 0; n < (NoPlaybackTimeout / SamplesPerFrame) - 1;
         n++) {
        meta_reader.set_flags(0);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }

    meta_reader.set_flags(0);
    expect_read(status::StatusAbort, watchdog, SamplesPerFrame);
}

TEST(watchdog, no_playback_timeout_blank_and_non_blank_frames) {
    CHECK(NoPlaybackTimeout % SamplesPerFrame == 0);

    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1), arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    for (unsigned int i = 0; i < 2; i++) {
        for (packet::stream_timestamp_t n = 0;
             n < (NoPlaybackTimeout / SamplesPerFrame) - 1; n++) {
            meta_reader.set_flags(0);
            expect_read(status::StatusOK, watchdog, SamplesPerFrame);
        }

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }
}

TEST(watchdog, no_playback_timeout_enabled_disabled) {
    { // enabled
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        for (packet::stream_timestamp_t n = 0;
             n < (NoPlaybackTimeout / SamplesPerFrame) - 1; n++) {
            meta_reader.set_flags(0);
            expect_read(status::StatusOK, watchdog, SamplesPerFrame);
        }

        meta_reader.set_flags(0);
        expect_read(status::StatusAbort, watchdog, SamplesPerFrame);
    }
    { // disabled
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(-1, BrokenPlaybackTimeout, -1), arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        for (packet::stream_timestamp_t n = 0;
             n < (NoPlaybackTimeout / SamplesPerFrame) - 1; n++) {
            meta_reader.set_flags(0);
            expect_read(status::StatusOK, watchdog, SamplesPerFrame);
        }

        meta_reader.set_flags(0);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }
}

TEST(watchdog, broken_playback_timeout_equal_frame_sizes) {
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_n_reads(status::StatusOK, watchdog, BreakageWindow,
                       BreakageWindowsPerTimeout - 1);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_n_reads(status::StatusOK, watchdog, BreakageWindow,
                       BreakageWindowsPerTimeout - 2);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal);
        expect_n_reads(status::StatusOK, watchdog, BreakageWindow,
                       BreakageWindowsPerTimeout);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_n_reads(status::StatusOK, watchdog, BreakageWindow,
                       BreakageWindowsPerTimeout - 1);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_n_reads(status::StatusOK, watchdog, BreakageWindow,
                       BreakageWindowsPerTimeout - 1);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps);
        expect_read(status::StatusOK, watchdog, BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_n_reads(status::StatusOK, watchdog, BreakageWindow,
                       BreakageWindowsPerTimeout - 1);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_n_reads(status::StatusOK, watchdog, BreakageWindow,
                       BreakageWindowsPerTimeout - 1);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusAbort, watchdog, BreakageWindow);
    }
}

TEST(watchdog, broken_playback_timeout_mixed_frame_sizes) {
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog,
                    BreakageWindow * (BreakageWindowsPerTimeout - 1));

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow / 2);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow - BreakageWindow / 2);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog,
                    BreakageWindow * (BreakageWindowsPerTimeout - 1));

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, BreakageWindow / 2);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusAbort, watchdog, BreakageWindow - BreakageWindow / 2);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog,
                    BreakageWindow * (BreakageWindowsPerTimeout - 1));

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow / 2);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusAbort, watchdog, BreakageWindow - BreakageWindow / 2);
    }
}

TEST(watchdog, broken_playback_timeout_constant_drops) {
    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1), arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    for (packet::stream_timestamp_t n = 0; n < BreakageWindowsPerTimeout - 1; n++) {
        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, BreakageWindow / 2);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow - BreakageWindow / 2);
    }

    meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
    expect_read(status::StatusOK, watchdog, BreakageWindow / 2);

    meta_reader.set_flags(Frame::HasSignal);
    expect_read(status::StatusAbort, watchdog, BreakageWindow - BreakageWindow / 2);
}

TEST(watchdog, broken_playback_timeout_frame_overlaps_with_breakage_window) {
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, BrokenPlaybackTimeout - BreakageWindow);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, BreakageWindow + 1);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow - 1);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusAbort, watchdog,
                    BrokenPlaybackTimeout - BreakageWindow);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BrokenPlaybackTimeout - BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, BreakageWindow + 1);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow - 1);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BrokenPlaybackTimeout - BreakageWindow);
    }
    {
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BrokenPlaybackTimeout - BreakageWindow);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, BreakageWindow + 1);

        meta_reader.set_flags(Frame::HasSignal);
        expect_read(status::StatusOK, watchdog, BreakageWindow - 1);

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusAbort, watchdog,
                    BrokenPlaybackTimeout - BreakageWindow);
    }
}

TEST(watchdog, broken_playback_timeout_enabled_disabled) {
    { // enabled
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1),
                          arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        for (packet::stream_timestamp_t n = 0;
             n < (BrokenPlaybackTimeout / SamplesPerFrame) - 1; n++) {
            meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
            expect_read(status::StatusOK, watchdog, SamplesPerFrame);
        }

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusAbort, watchdog, SamplesPerFrame);
    }
    { // disabled
        MetaReader meta_reader;
        Watchdog watchdog(meta_reader, sample_spec,
                          make_config(NoPlaybackTimeout, -1, -1), arena);
        LONGS_EQUAL(status::StatusOK, watchdog.init_status());

        for (packet::stream_timestamp_t n = 0;
             n < (BrokenPlaybackTimeout / SamplesPerFrame) - 1; n++) {
            meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
            expect_read(status::StatusOK, watchdog, SamplesPerFrame);
        }

        meta_reader.set_flags(Frame::HasSignal | Frame::HasGaps | Frame::HasDrops);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }
}

TEST(watchdog, warmup_shorter_than_timeout) {
    enum { Warmup = NoPlaybackTimeout / 2 };

    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, Warmup),
                      arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    for (packet::stream_timestamp_t n = 0; n < Warmup / SamplesPerFrame; n++) {
        meta_reader.set_flags(0);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }

    for (packet::stream_timestamp_t n = 0; n < NoPlaybackTimeout / SamplesPerFrame - 1;
         n++) {
        meta_reader.set_flags(0);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }

    meta_reader.set_flags(0);
    expect_read(status::StatusAbort, watchdog, SamplesPerFrame);
}

TEST(watchdog, warmup_longer_than_timeout) {
    enum { Warmup = NoPlaybackTimeout * 10 };

    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, Warmup),
                      arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    for (packet::stream_timestamp_t n = 0; n < Warmup / SamplesPerFrame; n++) {
        meta_reader.set_flags(0);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }

    for (packet::stream_timestamp_t n = 0; n < NoPlaybackTimeout / SamplesPerFrame - 1;
         n++) {
        meta_reader.set_flags(0);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }

    meta_reader.set_flags(0);
    expect_read(status::StatusAbort, watchdog, SamplesPerFrame);
}

TEST(watchdog, warmup_early_nonblank) {
    enum { Warmup = NoPlaybackTimeout * 10 };

    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, Warmup),
                      arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    for (packet::stream_timestamp_t n = 0; n < (Warmup / 2) / SamplesPerFrame; n++) {
        meta_reader.set_flags(0);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }

    meta_reader.set_flags(Frame::HasSignal);
    expect_read(status::StatusOK, watchdog, SamplesPerFrame);

    for (packet::stream_timestamp_t n = 0; n < (NoPlaybackTimeout / SamplesPerFrame) - 1;
         n++) {
        meta_reader.set_flags(0);
        expect_read(status::StatusOK, watchdog, SamplesPerFrame);
    }

    meta_reader.set_flags(0);
    expect_read(status::StatusAbort, watchdog, SamplesPerFrame);
}

// Forwarding mode to underlying reader.
TEST(watchdog, forward_mode) {
    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1), arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    const FrameReadMode mode_list[] = {
        ModeHard,
        ModeSoft,
    };

    for (size_t md_n = 0; md_n < ROC_ARRAY_SIZE(mode_list); md_n++) {
        FramePtr frame = frame_factory.allocate_frame_no_buffer();
        CHECK(frame);

        LONGS_EQUAL(status::StatusOK,
                    watchdog.read(*frame, SamplesPerFrame, mode_list[md_n]));

        LONGS_EQUAL(mode_list[md_n], meta_reader.last_mode());
    }
}

// Forwarding error from underlying reader.
TEST(watchdog, forward_error) {
    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1), arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    const status::StatusCode status_list[] = {
        status::StatusDrain,
        status::StatusAbort,
    };

    for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
        meta_reader.set_status(status_list[st_n]);

        FramePtr frame = frame_factory.allocate_frame_no_buffer();
        CHECK(frame);

        LONGS_EQUAL(status_list[st_n], watchdog.read(*frame, SamplesPerFrame, ModeHard));
        LONGS_EQUAL(status_list[st_n], meta_reader.last_status());
    }
}

// Forwarding partial read from underlying reader.
TEST(watchdog, forward_partial) {
    MetaReader meta_reader;
    Watchdog watchdog(meta_reader, sample_spec,
                      make_config(NoPlaybackTimeout, BrokenPlaybackTimeout, -1), arena);
    LONGS_EQUAL(status::StatusOK, watchdog.init_status());

    meta_reader.set_limit(SamplesPerFrame / 2);

    for (packet::stream_timestamp_t n = 0;
         n < ((NoPlaybackTimeout / SamplesPerFrame) - 1) * 2; n++) {
        meta_reader.set_flags(0);
        expect_read(status::StatusPart, watchdog, SamplesPerFrame, SamplesPerFrame / 2);
    }

    meta_reader.set_flags(0);
    expect_read(status::StatusPart, watchdog, SamplesPerFrame, SamplesPerFrame / 2);

    meta_reader.set_flags(0);
    expect_read(status::StatusAbort, watchdog, SamplesPerFrame);
}

} // namespace audio
} // namespace roc
