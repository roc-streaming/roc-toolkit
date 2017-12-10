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
    NumCh = 2,
    SamplesPerFrame = Timeout / 4,

    DropWindowSz = SamplesPerFrame,
    MaxDropWindowNum = DropWindowSz * 5
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
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    CHECK(watchdog.update(Timeout * 2));
    check_read(watchdog, true, SamplesPerFrame, 0);
}

TEST(watchdog, update_timeout) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    CHECK(watchdog.update(0));
    CHECK(!watchdog.update(Timeout));
}

TEST(watchdog, nth_update_timeout) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    for (packet::timestamp_t n = 0; n < Timeout; n++) {
        CHECK(watchdog.update(n));
    }

    CHECK(!watchdog.update(Timeout));
}

TEST(watchdog, empty_frames_timeout) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    for (packet::timestamp_t n = 0; n < Timeout; n++) {
        CHECK(watchdog.update(n));
        check_read(watchdog, true, SamplesPerFrame, Frame::FlagEmpty);
    }

    CHECK(!watchdog.update(Timeout));
    check_read(watchdog, false, SamplesPerFrame, 0);
}

TEST(watchdog, empty_frames_no_timeout) {
    CHECK(Timeout % SamplesPerFrame == 0);

    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    for (packet::timestamp_t ni = 0; ni < Timeout / SamplesPerFrame; ni++) {
        const packet::timestamp_t from = ni * Timeout;
        const packet::timestamp_t to = from + Timeout;

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

TEST(watchdog, frame_flags_packet_drops_begin) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    check_nth_read(watchdog, true, DropWindowSz, (MaxDropWindowNum / DropWindowSz) - 1,
                   Frame::FlagPacketDrops);

    check_read(watchdog, true, DropWindowSz, 0);
    CHECK(watchdog.update(Timeout));
    check_read(watchdog, true, DropWindowSz, 0);
}

TEST(watchdog, frame_flags_packet_drops_middle) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    check_read(watchdog, true, DropWindowSz, 0);
    check_nth_read(watchdog, true, DropWindowSz, (MaxDropWindowNum / DropWindowSz) - 2,
                   Frame::FlagPacketDrops);
    check_read(watchdog, true, DropWindowSz, 0);

    CHECK(watchdog.update(Timeout));
    check_nth_read(watchdog, true, DropWindowSz, (MaxDropWindowNum / DropWindowSz), 0);
}

TEST(watchdog, frame_flags_packet_drops_end) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    check_read(watchdog, true, DropWindowSz, 0);
    check_nth_read(watchdog, true, DropWindowSz, (MaxDropWindowNum / DropWindowSz) - 1,
                   Frame::FlagPacketDrops);

    CHECK(watchdog.update(Timeout));

    check_read(watchdog, true, DropWindowSz, 0);
}

TEST(watchdog, frame_flags_packet_drops_all) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    check_nth_read(watchdog, true, DropWindowSz, (MaxDropWindowNum / DropWindowSz) - 1,
                   Frame::FlagPacketDrops);
    check_read(watchdog, true, DropWindowSz, Frame::FlagPacketDrops);

    CHECK(!watchdog.update(Timeout));
    check_read(watchdog, false, DropWindowSz, 0);
}

TEST(watchdog, frame_flags_empty_packet_drops) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    check_read(watchdog, true, DropWindowSz, Frame::FlagPacketDrops);
    check_read(watchdog, true, DropWindowSz, Frame::FlagEmpty);

    CHECK(watchdog.update(Timeout));

    check_read(watchdog, true, DropWindowSz, 0);
}

TEST(watchdog, frame_flags_full_packet_drops) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    check_nth_read(watchdog, true, DropWindowSz, (MaxDropWindowNum / DropWindowSz),
                   Frame::FlagPacketDrops | Frame::FlagFull);

    CHECK(watchdog.update(Timeout));

    check_nth_read(watchdog, true, DropWindowSz, (MaxDropWindowNum / DropWindowSz), 0);
}

TEST(watchdog, frame_flags_packet_drops_reset) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    check_read(watchdog, true, DropWindowSz * ((MaxDropWindowNum / DropWindowSz) - 1),
               Frame::FlagPacketDrops);
    check_read(watchdog, true, DropWindowSz, 0);

    CHECK(watchdog.update(Timeout));

    check_read(watchdog, true, DropWindowSz * ((MaxDropWindowNum / DropWindowSz) - 1),
            Frame::FlagPacketDrops);
}

TEST(watchdog, frame_flags_packet_drops_window_exceeded_after_reset) {
    Watchdog watchdog(test_reader, NumCh, Timeout, DropWindowSz, MaxDropWindowNum);

    check_read(watchdog, true, DropWindowSz * ((MaxDropWindowNum / DropWindowSz) - 1),
               Frame::FlagPacketDrops);
    check_read(watchdog, true, DropWindowSz / 2, Frame::FlagPacketDrops);
    check_read(watchdog, true, DropWindowSz, 0);

    CHECK(!watchdog.update(Timeout));
}

} // namespace audio
} // namespace roc
