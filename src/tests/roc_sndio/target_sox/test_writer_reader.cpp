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
#include "roc_core/stddefs.h"
#include "roc_core/temp_file.h"
#include "roc_sndio/player.h"
#include "roc_sndio/sox.h"
#include "roc_sndio/sox_reader.h"
#include "roc_sndio/sox_writer.h"

namespace roc {
namespace sndio {

namespace {

enum { MaxBufSize = 8192, SampleRate = 44100, ChMask = 0x3, FrameSize = 512 };

core::HeapAllocator allocator;
core::BufferPool<audio::sample_t> buffer_pool(allocator, MaxBufSize, true);

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

TEST_GROUP(writer_reader) {
    void setup() {
        sox_setup();
    }
};

TEST(writer_reader, writer_noop) {
    SoxWriter writer(allocator, ChMask, SampleRate);
}

TEST(writer_reader, writer_error) {
    SoxWriter writer(allocator, ChMask, SampleRate);

    CHECK(!writer.open("/bad/file", NULL));
}

TEST(writer_reader, writer_start_stop) {
    SoxWriter writer(allocator, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(writer.open(file.path(), NULL));

    MockReceiver receiver;
    Player player(buffer_pool, receiver, writer, writer.frame_size(), true);

    CHECK(player.start());
    player.stop();
    player.join();
}

TEST(writer_reader, writer_stop_start) {
    SoxWriter writer(allocator, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(writer.open(file.path(), NULL));

    MockReceiver receiver;
    Player player(buffer_pool, receiver, writer, writer.frame_size(), true);

    player.stop();
    CHECK(player.start());
    player.join();
}

TEST(writer_reader, writer_start_start) {
    SoxWriter writer(allocator, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(writer.open(file.path(), NULL));

    MockReceiver receiver;
    Player player(buffer_pool, receiver, writer, writer.frame_size(), true);

    CHECK(player.start());
    CHECK(!player.start());
    player.stop();
    player.join();
}

TEST(writer_reader, writer_is_file) {
    SoxWriter writer(allocator, ChMask, 0);

    core::TempFile file("test.wav");
    CHECK(writer.open(file.path(), NULL));

    CHECK(writer.is_file());
}

TEST(writer_reader, writer_sample_rate_auto) {
    SoxWriter writer(allocator, ChMask, 0);

    core::TempFile file("test.wav");
    CHECK(writer.open(file.path(), NULL));
    CHECK(writer.sample_rate() != 0);
}

TEST(writer_reader, writer_sample_rate_force) {
    SoxWriter writer(allocator, ChMask, SampleRate);

    core::TempFile file("test.wav");
    CHECK(writer.open(file.path(), NULL));
    CHECK(writer.sample_rate() == SampleRate);
}

TEST(writer_reader, reader_noop) {
    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate);
}

TEST(writer_reader, reader_error) {
    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(!reader.open("/bad/file", NULL));
}

TEST(writer_reader, reader_start_stop) {
    enum { NumSamples = MaxBufSize * 10 };

    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(NumSamples);

        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();
    }

    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(reader.open(file.path(), NULL));

    MockWriter writer;

    CHECK(reader.start(writer));
    reader.stop();
    reader.join();
}

TEST(writer_reader, reader_stop_start) {
    enum { NumSamples = MaxBufSize * 10 };

    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(NumSamples);

        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();
    }

    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(reader.open(file.path(), NULL));

    MockWriter writer;

    reader.stop();
    CHECK(reader.start(writer));
    reader.join();
}

TEST(writer_reader, reader_start_start) {
    enum { NumSamples = MaxBufSize * 10 };

    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(NumSamples);

        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();
    }

    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(reader.open(file.path(), NULL));

    MockWriter writer;

    CHECK(reader.start(writer));
    CHECK(!reader.start(writer));
    reader.stop();
    reader.join();
}

TEST(writer_reader, reader_is_file) {
    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(MaxBufSize * 10);

        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();
    }

    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(reader.open(file.path(), NULL));
    CHECK(reader.is_file());
}

TEST(writer_reader, reader_sample_rate_auto) {
    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(MaxBufSize * 10);

        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();
    }

    SoxReader reader(buffer_pool, ChMask, FrameSize, 0);

    CHECK(reader.open(file.path(), NULL));
    CHECK(reader.sample_rate() == SampleRate);
}

TEST(writer_reader, reader_sample_rate_mismatch) {
    core::TempFile file("test.wav");

    {
        MockReceiver receiver;
        receiver.add(MaxBufSize * 10);

        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();
    }

    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate * 2);

    CHECK(reader.open(file.path(), NULL));
    CHECK(reader.sample_rate() == SampleRate);
}

TEST(writer_reader, write_read) {
    enum { NumSamples = MaxBufSize * 10 };

    MockReceiver receiver;
    receiver.add(NumSamples);

    core::TempFile file("test.wav");

    {
        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();

        CHECK(receiver.num_returned() >= NumSamples - MaxBufSize);
    }

    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate);
    CHECK(reader.open(file.path(), NULL));

    MockWriter writer;
    CHECK(reader.start(writer));
    reader.join();

    writer.check(0, receiver.num_returned());
}

TEST(writer_reader, overwrite) {
    enum { NumSamples = MaxBufSize * 10 };

    MockReceiver receiver;
    receiver.add(NumSamples);

    core::TempFile file("test.wav");

    {
        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();
    }

    receiver.add(NumSamples);

    size_t num_returned1 = receiver.num_returned();
    CHECK(num_returned1 >= NumSamples - MaxBufSize);

    {
        SoxWriter writer(allocator, ChMask, SampleRate);
        CHECK(writer.open(file.path(), NULL));

        Player player(buffer_pool, receiver, writer, writer.frame_size(), true);
        CHECK(player.start());
        player.join();
    }

    size_t num_returned2 = receiver.num_returned() - num_returned1;
    CHECK(num_returned1 >= NumSamples - MaxBufSize);

    SoxReader reader(buffer_pool, ChMask, FrameSize, SampleRate);

    CHECK(reader.open(file.path(), NULL));

    MockWriter writer;

    CHECK(reader.start(writer));
    reader.join();

    writer.check(num_returned1, num_returned2);
}

} // namespace sndio
} // namespace roc
