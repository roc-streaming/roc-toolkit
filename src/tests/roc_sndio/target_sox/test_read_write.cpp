/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include <stdlib.h>
#include <stdio.h>

#include "roc_core/stddefs.h"
#include "roc_core/math.h"
#include "roc_core/fs.h"
#include "roc_core/log.h"

#include "roc_audio/sample_buffer_queue.h"

#include "roc_sndio/reader.h"
#include "roc_sndio/writer.h"

namespace roc {
namespace test {

using namespace sndio;
using namespace audio;

using packet::sample_t;

namespace {

enum { ChLeft = (1 << 0), ChRight = (1 << 1) };

enum {
    NumSamples = ROC_CONFIG_DEFAULT_SERVER_TICK_SAMPLES / 4 + 3, //
    NumChannels = 2,                                             //
    ChannelMask = ChLeft | ChRight,                              //
    SampleRate = 44100                                           //
};

const sample_t Step = 0.0001f, Epsilon = 0.00001f;

typedef SampleBufferQueue Queue;

} // namespace

TEST_GROUP(read_write) {
    sample_t wr_pos, rd_pos;

    char temp_dir[512];
    char temp_file[512];

    void setup() {
        wr_pos = 0;
        rd_pos = 0;

        CHECK(core::create_temp_dir(temp_dir, sizeof(temp_dir)));
        snprintf(temp_file, sizeof(temp_file), "%s/sndio.wav", temp_dir);
    }

    void teardown() {
        remove(temp_file);
        CHECK(core::remove_dir(temp_dir));
    }

    ISampleBufferComposer& composer() {
        return default_buffer_composer();
    }

    void write_eof(ISampleBufferWriter & writer) {
        writer.write(ISampleBufferConstSlice());
    }

    void write_samples(ISampleBufferWriter & writer) {
        ISampleBufferPtr buffer = composer().compose();
        CHECK(buffer);

        buffer->set_size(NumSamples * NumChannels);

        for (size_t n = 0; n < buffer->size(); n++) {
            buffer->data()[n] = wr_pos;
            wr_pos += Step;
        }

        writer.write(*buffer);
    }

    void read_samples(ISampleBufferReader & reader) {
        ISampleBufferConstSlice buffer = reader.read();
        CHECK(buffer);

        LONGS_EQUAL(NumSamples * NumChannels, buffer.size());

        for (size_t n = 0; n < buffer.size(); n++) {
            const sample_t value = buffer.data()[n];
            if (ROC_ABS(rd_pos - value) > Epsilon) {
                buffer.print();
            }
            DOUBLES_EQUAL(rd_pos, value, Epsilon);
            rd_pos += Step;
        }
    }

    void write_file(size_t n_bufs = 10, size_t ch_mask = ChannelMask,
                    size_t sample_rate = SampleRate) {
        Queue input;
        Writer writer(input, (packet::channel_mask_t)ch_mask, sample_rate);

        CHECK(writer.open(temp_file));

        writer.start();

        for (size_t n = 0; n < n_bufs; n++) {
            write_samples(input);
        }

        write_eof(input);
        writer.join();

        LONGS_EQUAL(0, input.size());
    }
};

TEST(read_write, empty) {
    {
        Queue input;
        Writer writer(input, ChannelMask);

        CHECK(writer.open(temp_file));

        writer.start();
        write_eof(input);
        writer.join();

        LONGS_EQUAL(0, input.size());
    }

    {
        Queue output;
        Reader reader(output, composer(), ChannelMask, NumSamples);

        CHECK(reader.open(temp_file));

        reader.start();
        reader.join();

        CHECK(!output.read());

        LONGS_EQUAL(0, output.size());
    }
}

TEST(read_write, samples) {
    {
        Queue input;
        Writer writer(input, ChannelMask);

        CHECK(writer.open(temp_file));

        writer.start();

        for (size_t n = 0; n < 10; n++) {
            write_samples(input);
        }

        write_eof(input);
        writer.join();

        LONGS_EQUAL(0, input.size());
    }

    {
        Queue output;
        Reader reader(output, composer(), ChannelMask, NumSamples);

        CHECK(reader.open(temp_file));

        reader.start();

        for (size_t n = 0; n < 10; n++) {
            read_samples(output);
        }

        reader.join();

        CHECK(!output.read());

        LONGS_EQUAL(0, output.size());
    }
}

TEST(read_write, writer_stop_before) {
    Queue input;
    Writer writer(input, ChannelMask);

    CHECK(writer.open(temp_file));

    writer.stop();
    writer.start();
    writer.join();
}

TEST(read_write, reader_stop_before) {
    write_file();

    Queue output;
    Reader reader(output, composer(), ChannelMask, NumSamples);

    CHECK(reader.open(temp_file));

    reader.stop();
    reader.start();
    reader.join();

    CHECK(!output.read());

    LONGS_EQUAL(0, output.size());
}

TEST(read_write, writer_open_destroy) {
    Queue input;
    Writer writer(input, ChannelMask);

    CHECK(writer.open(temp_file));
}

TEST(read_write, reader_open_destroy) {
    write_file();

    Queue output;
    Reader reader(output, composer(), ChannelMask, NumSamples);

    CHECK(reader.open(temp_file));
}

TEST(read_write, writer_bad_args) {
    LogLevel level = core::set_log_level(LogNone);

    {
        Queue input;
        Writer writer(input, ChannelMask);

        CHECK(!writer.open("/bad/file"));
    }

    {
        Queue input;
        Writer writer(input, ChannelMask);

        CHECK(!writer.open(temp_file, "bad file format"));
    }

    {
        Queue input;
        Writer writer(input, ChannelMask);

        CHECK(writer.open(temp_file, "wav"));
    }

    core::set_log_level(level);
}

TEST(read_write, reader_bad_args) {
    LogLevel level = core::set_log_level(LogNone);

    write_file();

    {
        Queue output;
        Reader reader(output, composer(), ChannelMask, NumSamples);

        CHECK(!reader.open("/bad/file"));
    }

    {
        Queue output;
        Reader reader(output, composer(), ChannelMask, NumSamples);

        CHECK(!reader.open(temp_file, "bad file format"));
    }

    {
        Queue output;
        Reader reader(output, composer(), ChannelMask, NumSamples);

        CHECK(reader.open(temp_file, "wav"));
    }

    core::set_log_level(level);
}

TEST(read_write, overwrite_file) {
    write_file();
    write_file();
}

TEST(read_write, resample) {
    const size_t NumBufs = 20;

    write_file(NumBufs, ChannelMask, SampleRate);

    Queue scaled2x;

    {
        Reader reader(scaled2x, composer(), ChannelMask, NumSamples, SampleRate * 2);

        CHECK(reader.open(temp_file));

        reader.start();
        reader.join();
    }

    CHECK(scaled2x.size() >= NumBufs * 2 - 2);
    CHECK(scaled2x.size() <= NumBufs * 2 + 2);

    scaled2x.write(ISampleBufferConstSlice());

    {
        Writer writer(scaled2x, ChannelMask, SampleRate * 2);

        CHECK(writer.open(temp_file));

        writer.start();
        writer.join();
    }

    CHECK(!scaled2x.read());

    LONGS_EQUAL(0, scaled2x.size());

    Queue scaled1x;

    {
        Reader reader(scaled1x, composer(), ChannelMask, NumSamples, SampleRate);

        CHECK(reader.open(temp_file));

        reader.start();
        reader.join();
    }

    CHECK(scaled1x.size() >= NumBufs - 2);
    CHECK(scaled1x.size() <= NumBufs + 2);
}

TEST(read_write, remap_channels) {
    const size_t NumBufs = 20;

    write_file(NumBufs, ChLeft, SampleRate);

    Queue queue;

    {
        Reader reader(queue, composer(), ChLeft | ChRight, NumSamples, SampleRate);

        CHECK(reader.open(temp_file));

        reader.start();
        reader.join();
    }

    LONGS_EQUAL(NumBufs * 2 + 1, queue.size());

    for (size_t p = 0; p < NumBufs * 2; p++) {
        ISampleBufferConstSlice buffer = queue.read();
        CHECK(buffer);

        LONGS_EQUAL(NumSamples * NumChannels, buffer.size());

        for (size_t n = 0; n < buffer.size();) {
            DOUBLES_EQUAL(rd_pos, buffer.data()[n + 0], Epsilon);
            DOUBLES_EQUAL(rd_pos, buffer.data()[n + 1], Epsilon);
            n += 2;
            rd_pos += Step;
        }
    }

    CHECK(!queue.read());
}

} // namespace test
} // namespace roc
