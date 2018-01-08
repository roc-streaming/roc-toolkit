/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/macros.h"
#include "roc_core/stddefs.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/init.h"
#include "roc_sndio/player.h"
#include "roc_sndio/recorder.h"

namespace roc {
namespace sndio {

namespace {

enum { MaxBufSize = 1024 * 8, SampleRate = 44100, ChMask = 0x3, FrameSize = 512 };

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> buffer_pool(allocator, MaxBufSize, 1);

audio::sample_t nth_sample(size_t n) {
    return audio::sample_t(uint8_t(n)) / audio::sample_t(1 << 8);
}

class MockReceiver : public pipeline::IReceiver {
public:
    MockReceiver()
        : pos_(0)
        , size_(0) {
    }

    virtual Status status() const {
        if (pos_ >= size_) {
            return Inactive;
        } else {
            return Active;
        }
    }

    virtual void wait_active() const {
        FAIL("not implemented");
    }

    virtual void read(audio::Frame& frame) {
        size_t ns = frame.size();
        if (ns > size_ - pos_) {
            ns = size_ - pos_;
        }

        if (ns > 0) {
            memcpy(frame.data(), samples_ + pos_, ns * sizeof(audio::sample_t));
            pos_ += ns;
        }

        if (ns < frame.size()) {
            memset(frame.data() + ns, 0, (frame.size() - ns) * sizeof(audio::sample_t));
        }
    }

    void add(size_t sz) {
        CHECK(size_ + sz <= MaxSz);

        for (size_t n = 0; n < sz; n++) {
            samples_[size_] = nth_sample(size_);
            size_++;
        }
    }

    size_t num_returned() const {
        return pos_;
    }

private:
    enum { MaxSz = 256 * 1024 };

    audio::sample_t samples_[MaxSz];
    size_t pos_;
    size_t size_;
};

class MockWriter : public audio::IWriter {
public:
    MockWriter()
        : pos_(0) {
    }

    virtual void write(audio::Frame& frame) {
        CHECK(pos_ + frame.size() <= MaxSz);

        memcpy(samples_ + pos_, frame.data(), frame.size() * sizeof(audio::sample_t));
        pos_ += frame.size();
    }

    void check(size_t offset, size_t size) {
        UNSIGNED_LONGS_EQUAL(pos_, size);

        for (size_t n = 0; n < size; n++) {
            DOUBLES_EQUAL(samples_[n], nth_sample(offset + n), 0.0001);
        }
    }

private:
    enum { MaxSz = 256 * 1024 };

    audio::sample_t samples_[MaxSz];
    size_t pos_;
};

} // namespace

TEST_GROUP(player_recorder) {
    void setup() {
        init();
    }
};

TEST(player_recorder, player_noop) {
    Player player(buffer_pool, allocator, true, ChMask, SampleRate);
}

TEST(player_recorder, player_error) {
    Player player(buffer_pool, allocator, true, ChMask, SampleRate);

    CHECK(!player.open("/bad/file", NULL));
}

TEST(player_recorder, player_start_stop) {
    MockReceiver receiver;
    Player player(buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path(), NULL));

    CHECK(player.start(receiver));
    player.stop();
    player.join();
}

TEST(player_recorder, player_stop_start) {
    MockReceiver receiver;
    Player player(buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path(), NULL));

    player.stop();
    CHECK(player.start(receiver));
    player.join();
}

TEST(player_recorder, player_start_start) {
    MockReceiver receiver;
    Player player(buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path(), NULL));

    CHECK(player.start(receiver));
    CHECK(!player.start(receiver));
    player.stop();
    player.join();
}

TEST(player_recorder, player_is_file) {
    Player player(buffer_pool, allocator, true, ChMask, 0);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path(), NULL));

    CHECK(player.is_file());
}

TEST(player_recorder, player_sample_rate_auto) {
    Player player(buffer_pool, allocator, true, ChMask, 0);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path(), NULL));
    CHECK(player.sample_rate() != 0);
}

TEST(player_recorder, player_sample_rate_force) {
    Player player(buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path(), NULL));
    CHECK(player.sample_rate() == SampleRate);
}

TEST(player_recorder, recorder_noop) {
    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate);
}

TEST(player_recorder, recorder_error) {
    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(!recorder.open("/bad/file", NULL));
}

TEST(player_recorder, recorder_start_stop) {
    enum { NumSamples = MaxBufSize * 10 };

    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(NumSamples);

        Player player(buffer_pool, allocator, true, ChMask, SampleRate);
        CHECK(player.open(file.path(), NULL));

        CHECK(player.start(receiver));
        player.join();
    }

    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path(), NULL));

    MockWriter writer;

    CHECK(recorder.start(writer));
    recorder.stop();
    recorder.join();
}

TEST(player_recorder, recorder_stop_start) {
    enum { NumSamples = MaxBufSize * 10 };

    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(NumSamples);

        Player player(buffer_pool, allocator, true, ChMask, SampleRate);
        CHECK(player.open(file.path(), NULL));

        CHECK(player.start(receiver));
        player.join();
    }

    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path(), NULL));

    MockWriter writer;

    recorder.stop();
    CHECK(recorder.start(writer));
    recorder.join();
}

TEST(player_recorder, recorder_start_start) {
    enum { NumSamples = MaxBufSize * 10 };

    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(NumSamples);

        Player player(buffer_pool, allocator, true, ChMask, SampleRate);
        CHECK(player.open(file.path(), NULL));

        CHECK(player.start(receiver));
        player.join();
    }

    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path(), NULL));

    MockWriter writer;

    CHECK(recorder.start(writer));
    CHECK(!recorder.start(writer));
    recorder.stop();
    recorder.join();
}

TEST(player_recorder, recorder_is_file) {
    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(MaxBufSize * 10);

        Player player(buffer_pool, allocator, true, ChMask, SampleRate);

        CHECK(player.open(file.path(), NULL));

        CHECK(player.start(receiver));
        player.join();
    }

    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path(), NULL));
    CHECK(recorder.is_file());
}

TEST(player_recorder, recorder_sample_rate_auto) {
    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(MaxBufSize * 10);

        Player player(buffer_pool, allocator, true, ChMask, SampleRate);

        CHECK(player.open(file.path(), NULL));

        CHECK(player.start(receiver));
        player.join();
    }

    Recorder recorder(buffer_pool, ChMask, FrameSize, 0);

    CHECK(recorder.open(file.path(), NULL));
    CHECK(recorder.sample_rate() == SampleRate);
}

TEST(player_recorder, recorder_sample_rate_force) {
    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(MaxBufSize * 10);

        Player player(buffer_pool, allocator, true, ChMask, SampleRate);

        CHECK(player.open(file.path(), NULL));

        CHECK(player.start(receiver));
        player.join();
    }

    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate * 2);

    CHECK(recorder.open(file.path(), NULL));
    CHECK(recorder.sample_rate() == SampleRate * 2);
}

TEST(player_recorder, write_read) {
    enum { NumSamples = MaxBufSize * 10 };

    MockReceiver receiver;
    receiver.add(NumSamples);

    Player player(buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path(), NULL));

    CHECK(player.start(receiver));
    player.join();

    CHECK(receiver.num_returned() >= NumSamples - MaxBufSize);

    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path(), NULL));

    MockWriter writer;

    CHECK(recorder.start(writer));
    recorder.join();

    writer.check(0, receiver.num_returned());
}

TEST(player_recorder, overwrite) {
    enum { NumSamples = MaxBufSize * 10 };

    core::TempFile file("test.wav");

    MockReceiver receiver;
    receiver.add(NumSamples);

    {
        Player player(buffer_pool, allocator, true, ChMask, SampleRate);
        CHECK(player.open(file.path(), NULL));

        CHECK(player.start(receiver));
        player.join();
    }

    receiver.add(NumSamples);

    size_t num_returned1 = receiver.num_returned();
    CHECK(num_returned1 >= NumSamples - MaxBufSize);

    {
        Player player(buffer_pool, allocator, true, ChMask, SampleRate);
        CHECK(player.open(file.path(), NULL));

        CHECK(player.start(receiver));
        player.join();
    }

    size_t num_returned2 = receiver.num_returned() - num_returned1;
    CHECK(num_returned1 >= NumSamples - MaxBufSize);

    Recorder recorder(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path(), NULL));

    MockWriter writer;

    CHECK(recorder.start(writer));
    recorder.join();

    writer.check(num_returned1, num_returned2);
}

} // namespace sndio
} // namespace roc
