/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/resampler.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/random.h"
#include "roc_core/stddefs.h"

#include "test_fft.h"
#include "test_mock_reader.h"

namespace roc {
namespace audio {

namespace {

enum {
    MaxSize = 4096,
    ResamplerFIRLen = 200,
    FrameSize = 512,
    OutSamples = FrameSize * 100 + 1,
    InSamples = OutSamples + (FrameSize * 3)
};

core::HeapAllocator allocator;
core::BufferPool<sample_t> buffer_pool(allocator, MaxSize, 1);

} // namespace

TEST_GROUP(resampler) {
    ResamplerConfig config;

    void setup() {
        config.window_size = ResamplerFIRLen;
        config.frame_size = FrameSize;
    }

    core::Slice<sample_t> new_buffer(size_t sz) {
        core::Slice<sample_t> buf = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);
        buf.resize(sz);
        return buf;
    }

    // Reads signal from the resampler and puts its spectrum into @p spectrum.
    // Spectrum must have twice bigger space than the length of the input signal.
    void get_sample_spectrum1(IReader & reader, double* spectrum, const size_t sig_len) {
        Frame frame;
        frame.samples = new_buffer(sig_len);
        reader.read(frame);
        UNSIGNED_LONGS_EQUAL(sig_len, frame.samples.size());
        for (size_t i = 0; i < sig_len; ++i) {
            spectrum[i * 2] = (double)frame.samples.data()[i];
            spectrum[i * 2 + 1] = 0; // imaginary part
        }
        FreqSpectrum(spectrum, sig_len);
    }

    // Reads signal from the resampler and puts its spectrum into @p spectrum.
    // Spectrum must have twice bigger space than the length of the input signal.
    void get_sample_spectrum2(IReader & reader, double* spectrum1, double* spectrum2,
                              size_t sig_len) {
        enum { nChannels = 2 };
        Frame frame;
        frame.samples = new_buffer(sig_len);
        reader.read(frame);
        UNSIGNED_LONGS_EQUAL(sig_len, frame.samples.size());
        size_t i = 0;
        for (; i < sig_len / nChannels; ++i) {
            spectrum1[i * 2] = (double)frame.samples.data()[i * nChannels];
            spectrum1[i * 2 + 1] = 0; // imaginary part
            spectrum2[i * 2] = (double)frame.samples.data()[i * nChannels + 1];
            spectrum2[i * 2 + 1] = 0; // imaginary part
        }
        memset(&spectrum1[i * 2], 0, (sig_len * 2 - i * 2) * sizeof(double));
        memset(&spectrum2[i * 2], 0, (sig_len * 2 - i * 2) * sizeof(double));
        FreqSpectrum(spectrum1, sig_len / nChannels);
        FreqSpectrum(spectrum2, sig_len / nChannels);
    }

    // Generates additive white Gaussian Noise samples with zero mean and a standard
    // deviation of 1.
    // https://www.embeddedrelated.com/showcode/311.php
    double AWGN_generator() {
        const double epsilon = 1.0 / (double)RAND_MAX;
        double temp2 = epsilon;

        int p = 1;
        while (p > 0) {
            temp2 = (rand() / ((double)RAND_MAX));
            if (temp2 <= epsilon) {
                // temp2 is >= (RAND_MAX / 2)
                p = 1;
            } else {
                p = -1;
            }
        }

        double temp1 = cos((2.0 * M_PI) * rand() / ((double)RAND_MAX));
        double result = sqrt(-2.0 * log(temp2)) * temp1;

        return result; // return the generated random sample to the caller
    }
};

TEST(resampler, invalid_scaling) {
    enum { ChMask = 0x1, InvalidScaling = FrameSize };

    MockReader reader;
    Resampler resampler(reader, buffer_pool, allocator, config, ChMask);

    CHECK(!resampler.set_scaling(InvalidScaling));
}

// Check the quality of upsampled sine-wave.
TEST(resampler, upscaling_twice_single) {
    enum { ChMask = 0x1 };

    MockReader reader;
    Resampler resampler(reader, buffer_pool, allocator, config, ChMask);

    CHECK(resampler.set_scaling(0.5f));

    const size_t sig_len = 2048;
    double buff[sig_len * 2];

    for (size_t n = 0; n < InSamples; n++) {
        const sample_t s = (sample_t)sin(M_PI / 4 * double(n));
        reader.add(1, s);
    }

    // Put the spectrum of the resampled signal into buff.
    // Odd elements are magnitudes in dB, even elements are phases in radians.
    get_sample_spectrum1(resampler, buff, sig_len);

    const size_t main_freq_index = sig_len / 8;
    for (size_t n = 0; n < sig_len / 2; n += 2) {
        // The main sinewave frequency decreased twice as we've upsampled.
        // So here SNR is checked.
        CHECK((buff[n] - buff[main_freq_index]) <= -110 || n == main_freq_index);
    }
}

// Check upsampling quality and the cut-off band with white noise.
TEST(resampler, upscaling_twice_awgn) {
    enum { ChMask = 0x1 };

    MockReader reader;
    Resampler resampler(reader, buffer_pool, allocator, config, ChMask);

    CHECK(resampler.set_scaling(0.5f));

    const size_t sig_len = 2048;
    double buff[sig_len * 2];
    size_t i;

    for (size_t n = 0; n < InSamples; n++) {
        const sample_t s = (sample_t)AWGN_generator();
        reader.add(1, s);
    }

    // Put the spectrum of the resampled signal into buff.
    // Odd elements are magnitudes in dB, even elements are phases in radians.
    get_sample_spectrum1(resampler, buff, sig_len);

    for (i = 0; i < sig_len - 1; i += 2) {
        if (i <= sig_len * 0.90 / 2) {
            CHECK(buff[i] >= -60);
        } else if (i >= sig_len * 1.00 / 2) {
            CHECK(buff[i] <= -60);
        }
    }
}

TEST(resampler, downsample) {
    enum { ChMask = 0x1 };

    MockReader reader;
    Resampler resampler(reader, buffer_pool, allocator, config, ChMask);

    CHECK(resampler.set_scaling(1.5f));
    const size_t sig_len = 2048;
    double buff[sig_len * 2];

    for (size_t n = 0; n < InSamples; n++) {
        const sample_t s = (sample_t)sin(M_PI / 4 * double(n));
        reader.add(1, s);
    }

    // Put the spectrum of the resampled signal into buff.
    // Odd elements are magnitudes in dB, even elements are phases in radians.
    get_sample_spectrum1(resampler, buff, sig_len);

    const size_t main_freq_index = (size_t)round(sig_len / 4 * 1.5);
    for (size_t n = 0; n < sig_len / 2; n += 2) {
        // The main sinewave frequency increased by 1.5 as we've downsampled.
        // So here SNR is checked.
        CHECK((buff[n] - buff[main_freq_index]) <= -110 || buff[n] < -200
              || n == main_freq_index);
    }
}

TEST(resampler, two_tones_sep_channels) {
    enum { ChMask = 0x3, nChannels = 2 };

    MockReader reader;
    Resampler resampler(reader, buffer_pool, allocator, config, ChMask);

    CHECK(resampler.set_scaling(0.5f));

    const size_t sig_len = 2048;
    double buff1[sig_len * 2];
    double buff2[sig_len * 2];
    size_t i;

    for (size_t n = 0; n < InSamples / nChannels; n++) {
        const sample_t s1 = (sample_t)sin(M_PI / 4 * double(n));
        const sample_t s2 = (sample_t)sin(M_PI / 8 * double(n));
        reader.add(1, s1);
        reader.add(1, s2);
    }

    // Put the spectrum of the resampled signal into buff.
    // Odd elements are magnitudes in dB, even elements are phases in radians.
    get_sample_spectrum2(resampler, buff1, buff2, sig_len);

    const size_t main_freq_index1 = sig_len / 8 / nChannels;
    const size_t main_freq_index2 = sig_len / 16 / nChannels;
    for (i = 0; i < sig_len / 2; i += 2) {
        CHECK((buff1[i] - buff1[main_freq_index1]) <= -75 || i == main_freq_index1);
        CHECK((buff2[i] - buff2[main_freq_index2]) <= -75 || i == main_freq_index2);
    }
}

} // namespace audio
} // namespace roc
