/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
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

    virtual Status read(audio::Frame& frame) {
        if (pos_ + frame.samples.size() >= size_) {
            return Inactive;
        }

        memcpy(frame.samples.data(), samples_ + pos_,
               frame.samples.size() * sizeof(audio::sample_t));

        pos_ += frame.samples.size();

        return Active;
    }

    virtual void wait_active() {
        FAIL("not implemented");
    }

    void add(size_t size) {
        CHECK(size_ + size < MaxSz);

        for (size_t n = 0; n < size; n++) {
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
        CHECK(pos_ + frame.samples.size() < MaxSz);

        memcpy(samples_ + pos_, frame.samples.data(),
               frame.samples.size() * sizeof(audio::sample_t));

        pos_ += frame.samples.size();
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
    MockReceiver receiver;
    Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);
}

TEST(player_recorder, player_error) {
    MockReceiver receiver;
    Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);

    CHECK(!player.open("/bad/file"));
}

TEST(player_recorder, player_start_stop) {
    MockReceiver receiver;
    Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path()));

    player.start();
    player.stop();
    player.join();
}

TEST(player_recorder, player_stop_start) {
    MockReceiver receiver;
    Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path()));

    player.stop();
    player.start();
    player.join();
}

TEST(player_recorder, recorder_noop) {
    MockWriter writer;
    Recorder recorder(writer, buffer_pool, ChMask, FrameSize, SampleRate);
}

TEST(player_recorder, recorder_error) {
    MockWriter writer;
    Recorder recorder(writer, buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(!recorder.open("/bad/file"));
}

TEST(player_recorder, recorder_start_stop) {
    enum { NumSamples = MaxBufSize * 10 };

    MockReceiver receiver;
    receiver.add(NumSamples);

    Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path()));

    player.start();
    player.join();

    MockWriter writer;
    Recorder recorder(writer, buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path()));

    recorder.start();
    recorder.stop();
    recorder.join();
}

TEST(player_recorder, recorder_stop_start) {
    enum { NumSamples = MaxBufSize * 10 };

    MockReceiver receiver;
    receiver.add(NumSamples);

    Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path()));

    player.start();
    player.join();

    MockWriter writer;
    Recorder recorder(writer, buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path()));

    recorder.stop();
    recorder.start();
    recorder.join();
}

TEST(player_recorder, write_read) {
    enum { NumSamples = MaxBufSize * 10 };

    MockReceiver receiver;
    receiver.add(NumSamples);

    Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(player.open(file.path()));

    player.start();
    player.join();

    CHECK(receiver.num_returned() >= NumSamples - MaxBufSize);

    MockWriter writer;
    Recorder recorder(writer, buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path()));

    recorder.start();
    recorder.join();

    writer.check(0, receiver.num_returned());
}

TEST(player_recorder, overwrite) {
    enum { NumSamples = MaxBufSize * 10 };

    core::TempFile file("test.wav");

    MockReceiver receiver;
    receiver.add(NumSamples);

    {
        Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);
        CHECK(player.open(file.path()));

        player.start();
        player.join();
    }

    receiver.add(NumSamples);

    size_t num_returned1 = receiver.num_returned();
    CHECK(num_returned1 >= NumSamples - MaxBufSize);

    {
        Player player(receiver, buffer_pool, allocator, true, ChMask, SampleRate);
        CHECK(player.open(file.path()));

        player.start();
        player.join();
    }

    size_t num_returned2 = receiver.num_returned() - num_returned1;
    CHECK(num_returned1 >= NumSamples - MaxBufSize);

    MockWriter writer;
    Recorder recorder(writer, buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(recorder.open(file.path()));

    recorder.start();
    recorder.join();

    writer.check(num_returned1, num_returned2);
}

} // namespace sndio
} // namespace roc
