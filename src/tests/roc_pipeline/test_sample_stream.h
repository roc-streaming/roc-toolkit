/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_SAMPLE_STREAM_H_
#define ROC_PIPELINE_TEST_SAMPLE_STREAM_H_

#include <CppUTest/TestHarness.h>

#include "roc_core/log.h"
#include "roc_core/math.h"
#include "roc_packet/units.h"

#include "roc_audio/isample_buffer_reader.h"
#include "roc_audio/isample_buffer_writer.h"

#include "test_config.h"
#include "test_helpers.h"

namespace roc {
namespace test {

class SampleStream {
public:
    enum { ReadBufsz = 20, MaxSamples = 1000 };

    SampleStream()
        : next_value_(1)
        , n_sessions_(1)
        , n_reads_(0) {
    }

    void read_zeros(audio::ISampleBufferReader& reader, size_t n_samples) {
        CHECK(n_samples % ReadBufsz == 0);

        for (size_t n = 0; n < n_samples / ReadBufsz; n++) {
            read_buffer(reader, 0, 0);
        }
    }

    void read(audio::ISampleBufferReader& reader, size_t n_samples) {
        CHECK(n_samples % ReadBufsz == 0);

        for (size_t n = 0; n < n_samples / ReadBufsz; n++) {
            next_value_ = read_buffer(reader, next_value_, n_sessions_);
        }
    }

    void write(audio::ISampleBufferWriter& writer, size_t n_samples) {
        audio::ISampleBufferPtr buffer = new_sample_buffer<MaxSamples>();
        buffer->set_size(n_samples * NumChannels);

        size_t pos = 0;
        for (size_t n = 0; n < n_samples; n++) {
            packet::sample_t s =
                packet::sample_t(next_value_ % MaxSampleValue) / MaxSampleValue;

            buffer->data()[pos++] = -s;
            buffer->data()[pos++] = +s;

            next_value_++;
        }

        writer.write(*buffer);
    }

    void advance(size_t n_samples) {
        next_value_ += n_samples;
    }

    void set_sessions(long sessions) {
        n_sessions_ = sessions;
    }

private:
    long read_buffer(audio::ISampleBufferReader& reader, long val, long mul) {
        audio::ISampleBufferConstSlice buffer = reader.read();

        CHECK(buffer);
        LONGS_EQUAL(ReadBufsz * NumChannels, buffer.size());

        size_t pos = 0;
        for (size_t n = 0; n < ReadBufsz; n++) {
            packet::sample_t s = packet::sample_t(val % MaxSampleValue) / MaxSampleValue;

            expect_sample(buffer, pos++, -s * mul);
            expect_sample(buffer, pos++, +s * mul);

            val++;
        }

        n_reads_++;
        return val;
    }

    void expect_sample(const audio::ISampleBufferConstSlice& buffer,
                       size_t pos,
                       packet::sample_t expected) {
        const float Epsilon = 0.0001f;

        packet::sample_t actual = buffer.data()[pos];

        if (ROC_ABS(actual - expected) > Epsilon) {
            roc_log(LogError,
                    "unexpected sample at pos %u (buffer # %ld):", (unsigned)pos,
                    n_reads_);
            buffer.print();
        }

        DOUBLES_EQUAL(expected, actual, Epsilon);
    }

    long next_value_;

    long n_sessions_;
    long n_reads_;
};

} // namespace test
} // namespace roc

#endif // ROC_PIPELINE_TEST_SAMPLE_STREAM_H_
