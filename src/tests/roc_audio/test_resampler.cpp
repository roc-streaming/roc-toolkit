/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/awgn.h"
#include "test_helpers/fft.h"
#include "test_helpers/median.h"
#include "test_helpers/mock_reader.h"

#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_reader.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

enum {
    MaxSize = 4000,

    ResamplerFIRLen = 200,
    FrameSize = 512,

    OutSamples = FrameSize * 100 + 1,
    InSamples = OutSamples + (FrameSize * 3)
};

core::HeapAllocator allocator;
core::BufferPool<sample_t> buffer_pool(allocator, MaxSize, true);

} // namespace

TEST_GROUP(resampler) {
    ResamplerConfig config;

    void setup() {
        config.window_interp = 512;
        config.window_size = ResamplerFIRLen;
    }

    core::Slice<sample_t> new_buffer(size_t sz) {
        core::Slice<sample_t> buf = new (buffer_pool) core::Buffer<sample_t>(buffer_pool);
        buf.resize(sz);
        return buf;
    }

    // Reads signal from the resampler and puts its spectrum into @p spectrum.
    // Spectrum must have twice bigger space than the length of the input signal.
    void get_sample_spectrum1(IReader & reader, double* spectrum, const size_t sig_len) {
        core::Slice<sample_t> buf = new_buffer(sig_len);

        Frame frame(buf.data(), buf.size());
        CHECK(reader.read(frame));

        for (size_t i = 0; i < sig_len; ++i) {
            spectrum[i * 2] = (double)frame.data()[i];
            spectrum[i * 2 + 1] = 0; // imaginary part
        }

        test::freq_spectrum(spectrum, sig_len);
    }

    // Reads signal from the resampler and puts its spectrum into @p spectrum.
    // Spectrum must have twice bigger space than the length of the input signal.
    void get_sample_spectrum2(IReader & reader, double* spectrum1, double* spectrum2,
                              size_t sig_len) {
        enum { nChannels = 2 };

        core::Slice<sample_t> buf = new_buffer(sig_len);

        Frame frame(buf.data(), buf.size());
        CHECK(reader.read(frame));

        size_t i = 0;
        for (; i < sig_len / nChannels; ++i) {
            spectrum1[i * 2] = (double)frame.data()[i * nChannels];
            spectrum1[i * 2 + 1] = 0; // imaginary part
            spectrum2[i * 2] = (double)frame.data()[i * nChannels + 1];
            spectrum2[i * 2 + 1] = 0; // imaginary part
        }

        memset(&spectrum1[i * 2], 0, (sig_len * 2 - i * 2) * sizeof(double));
        memset(&spectrum2[i * 2], 0, (sig_len * 2 - i * 2) * sizeof(double));

        test::freq_spectrum(spectrum1, sig_len / nChannels);
        test::freq_spectrum(spectrum2, sig_len / nChannels);
    }
};

TEST(resampler, invalid_scaling) {
    enum { ChMask = 0x1, InvalidScaling = FrameSize };

    const core::nanoseconds_t FrameDuration =
        FrameSize * core::Second / (InSamples * packet::num_channels(ChMask));

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        test::MockReader reader;
        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(backend, allocator, config,
                                                   FrameDuration, InSamples, ChMask),
            allocator);
        CHECK(resampler);
        ResamplerReader rr(reader, *resampler, buffer_pool, FrameDuration, InSamples,
                           ChMask);
        CHECK(rr.valid());

        CHECK(!rr.set_scaling(InvalidScaling));
    }
}

// Check the quality of upsampled sine-wave.
TEST(resampler, upscaling_twice_single) {
    enum { ChMask = 0x1 };
    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        const core::nanoseconds_t FrameDuration =
            FrameSize * core::Second / (InSamples * packet::num_channels(ChMask));

        test::MockReader reader;
        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(backend, allocator, config,
                                                   FrameDuration, InSamples, ChMask),
            allocator);
        CHECK(resampler);
        ResamplerReader rr(reader, *resampler, buffer_pool, FrameDuration, InSamples,
                           ChMask);

        CHECK(rr.valid());

        CHECK(rr.set_scaling(0.5f));

        const size_t sig_len = 2048;
        double buff[sig_len * 2];

        for (size_t n = 0; n < InSamples; n++) {
            const sample_t s = (sample_t)std::sin(M_PI / 4 * double(n));
            reader.add(1, s);
        }

        // Put the spectrum of the resampled signal into buff.
        // Odd elements are magnitudes in dB, even elements are phases in radians.
        get_sample_spectrum1(rr, buff, sig_len);

        const size_t main_freq_index = sig_len / 8;
        for (size_t n = 0; n < sig_len / 2; n += 2) {
            // The main sinewave frequency decreased twice as we've upsampled.
            // So here SNR is checked.
            CHECK((buff[n] - buff[main_freq_index]) <= -110 || n == main_freq_index);
        }
    }
}

// Check upsampling quality and the cut-off band with white noise.
TEST(resampler, upscaling_twice_awgn) {
    enum { ChMask = 0x1 };

    const core::nanoseconds_t FrameDuration =
        FrameSize * core::Second / (InSamples * packet::num_channels(ChMask));

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        test::MockReader reader;
        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(backend, allocator, config,
                                                   FrameDuration, InSamples, ChMask),
            allocator);
        CHECK(resampler);
        ResamplerReader rr(reader, *resampler, buffer_pool, FrameDuration, InSamples,
                           ChMask);

        CHECK(rr.valid());
        CHECK(rr.set_scaling(0.5f));

        // Generate white noise.
        for (size_t n = 0; n < InSamples; n++) {
            const sample_t s = (sample_t)test::generate_awgn();
            reader.add(1, s);
        }

        // Put the spectrum of the resampled signal into buff.
        // Odd elements are magnitudes in dB, even elements are phases in radians.
        const size_t sig_len = 2048;
        double buff[sig_len * 2];
        get_sample_spectrum1(rr, buff, sig_len);

        // Get dB part.
        const size_t db_len = sig_len / 2;
        double db[db_len];
        for (size_t i = 0; i < sig_len; i += 2) {
            db[i / 2] = buff[i];
        }

        // Remove spikes using median filter.
        double filtered_db[db_len];
        test::median_filter(db, filtered_db, db_len);

        for (size_t i = 0; i < db_len; i++) {
            if (i <= db_len * 0.4) {
                CHECK(filtered_db[i] >= -50);
            } else if (i >= db_len * 0.8) {
                CHECK(filtered_db[i] <= -50);
            }
        }
    }
}

TEST(resampler, downsample) {
    enum { ChMask = 0x1 };

    const core::nanoseconds_t FrameDuration =
        FrameSize * core::Second / (InSamples * packet::num_channels(ChMask));

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        test::MockReader reader;
        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(backend, allocator, config,
                                                   FrameDuration, InSamples, ChMask),
            allocator);
        CHECK(resampler);
        ResamplerReader rr(reader, *resampler, buffer_pool, FrameDuration, InSamples,
                           ChMask);

        CHECK(rr.valid());
        CHECK(rr.set_scaling(1.5f));

        const size_t sig_len = 2048;
        double buff[sig_len * 2];

        for (size_t n = 0; n < InSamples; n++) {
            const sample_t s = (sample_t)std::sin(M_PI / 4 * double(n));
            reader.add(1, s);
        }

        // Put the spectrum of the resampled signal into buff.
        // Odd elements are magnitudes in dB, even elements are phases in radians.
        get_sample_spectrum1(rr, buff, sig_len);

        const size_t main_freq_index = (size_t)round(sig_len / 4 * 1.5);
        for (size_t n = 0; n < sig_len / 2; n += 2) {
            // The main sinewave frequency increased by 1.5 as we've downsampled.
            // So here SNR is checked.
            CHECK((buff[n] - buff[main_freq_index]) <= -110 || buff[n] < -200
                  || n == main_freq_index);
        }
    }
}

TEST(resampler, two_tones_sep_channels) {
    enum { ChMask = 0x3, nChannels = 2 };

    const core::nanoseconds_t FrameDuration =
        FrameSize * core::Second / (InSamples * packet::num_channels(ChMask));

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        test::MockReader reader;
        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(backend, allocator, config,
                                                   FrameDuration, InSamples, ChMask),
            allocator);
        CHECK(resampler);
        ResamplerReader rr(reader, *resampler, buffer_pool, FrameDuration, InSamples,
                           ChMask);
        CHECK(rr.valid());
        CHECK(rr.set_scaling(0.5f));

        const size_t sig_len = 2048;
        double buff1[sig_len * 2];
        double buff2[sig_len * 2];
        size_t i;

        for (size_t n = 0; n < InSamples / nChannels; n++) {
            const sample_t s1 = (sample_t)std::sin(M_PI / 4 * double(n));
            const sample_t s2 = (sample_t)std::sin(M_PI / 8 * double(n));
            reader.add(1, s1);
            reader.add(1, s2);
        }

        // Put the spectrum of the resampled signal into buff.
        // Odd elements are magnitudes in dB, even elements are phases in radians.
        get_sample_spectrum2(rr, buff1, buff2, sig_len);

        const size_t main_freq_index1 = sig_len / 8 / nChannels;
        const size_t main_freq_index2 = sig_len / 16 / nChannels;
        for (i = 0; i < sig_len / 2; i += 2) {
            CHECK((buff1[i] - buff1[main_freq_index1]) <= -75 || i == main_freq_index1);
            CHECK((buff2[i] - buff2[main_freq_index2]) <= -75 || i == main_freq_index2);
        }
    }
}

} // namespace audio
} // namespace roc
