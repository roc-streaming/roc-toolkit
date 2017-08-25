/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <CppUTest/TestHarness.h>

#include "roc_audio/resampler.h"
#include "roc_config/config.h"
#include "roc_core/scoped_ptr.h"
#include "fft.h"
#include "test_stream_reader.h"
#include "roc_core/helpers.h"

namespace roc {
namespace test {

using namespace audio;

namespace {

enum { FrameSize = ROC_CONFIG_DEFAULT_RESAMPLER_FRAME_SAMPLES*2};
enum { ResamplerFIRLen = 200 };
enum { nChannels = 2 };

enum { OutSamples = FrameSize * 100 + 1, InSamples = OutSamples + (FrameSize * 3) };

} // namespace

TEST_GROUP(multichannel_resampler) {
    TestStreamReader<InSamples> reader;

    core::ScopedPtr<Resampler> resampler;

    void setup() {
        resampler.reset(new Resampler(reader, default_buffer_composer(), ResamplerFIRLen, FrameSize, nChannels));
    }

    // Reads signal from the resampler and puts its spectrum into @p spectrum.
    //
    // Spectrum must have twice bigger space than the length of the input signal.
    void get_sample_spectrum(double *spectrum1, double *spectrum2, const size_t sig_len) {
        audio::ISampleBufferPtr buf = new_buffer<InSamples>(sig_len);
        resampler->read(*buf);
        LONGS_EQUAL(sig_len, buf->size());
        size_t i = 0;
        for (; i < sig_len/nChannels; ++i) {
            spectrum1[i*2] = (double)buf->data()[i*nChannels];
            spectrum1[i*2 + 1] = 0; // imagenary part.
            spectrum2[i*2] = (double)buf->data()[i*nChannels + 1];
            spectrum2[i*2 + 1] = 0; // imagenary part.
        }
        memset(&spectrum1[i*2], 0, (sig_len*2 - i*2)*sizeof(double));
        memset(&spectrum2[i*2], 0, (sig_len*2 - i*2)*sizeof(double));
        FreqSpectrum(spectrum1, sig_len/nChannels);
        FreqSpectrum(spectrum2, sig_len/nChannels);
    }
};

TEST(multichannel_resampler, two_tones_sep_channels) {
    CHECK(resampler->set_scaling(0.5f));
    const size_t sig_len = 2048;
    double buff1[sig_len*2];
    double buff2[sig_len*2];
    size_t i;

    for (size_t n = 0; n < InSamples/nChannels; n++) {
        const packet::sample_t s1 = (packet::sample_t)sin(M_PI/4 * double(n));
        const packet::sample_t s2 = (packet::sample_t)sin(M_PI/8 * double(n));
        reader.add(1, s1);
        reader.add(1, s2);
    }

    // Put the spectrum of the resampled signal into buff.
    // Odd elements are magnitudes in dB, even elements are phases in radians.
    get_sample_spectrum(buff1, buff2, sig_len);

    const size_t main_freq_index1 = sig_len / 8 / nChannels;
    const size_t main_freq_index2 = sig_len / 16 / nChannels;
    for (i = 0; i < sig_len/2; i += 2) {
        CHECK((buff1[i] - buff1[main_freq_index1]) <= -75 || i == main_freq_index1);
        CHECK((buff2[i] - buff2[main_freq_index2]) <= -75 || i == main_freq_index2);
    }
}

} // namespace test
} // namespace roc
