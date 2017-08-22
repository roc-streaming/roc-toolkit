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

enum { FrameSize = ROC_CONFIG_DEFAULT_RESAMPLER_FRAME_SAMPLES};
enum { ResamplerFIRLen = 200 };
enum { nChannels = 2 };

enum { OutSamples = FrameSize * 100 + 1, InSamples = OutSamples + (FrameSize * 3) };

} // namespace

TEST_GROUP(multichannel_resampler) {
    TestStreamReader<InSamples> reader;

    core::ScopedPtr<Resampler> resampler;

    void setup() {
        resampler.reset(new Resampler(reader, default_buffer_composer(), ResamplerFIRLen, FrameSize*nChannels, nChannels));
    }

    // Reads signal from the resampler and puts its spectrum into @p spectrum.
    //
    // Spectrum must have twice bigger space than the length of the input signal.
    // @p nchannel is the channel offset.
    void get_sample_spectrum(double *spectrum, const size_t sig_len, size_t nchannel = 0) {
        audio::ISampleBufferPtr buf = new_buffer<InSamples>(sig_len);
        resampler->read(*buf);
        LONGS_EQUAL(sig_len, buf->size());
        for (size_t i = 0; i < sig_len; ++i) {
            spectrum[i*2] = (double)buf->data()[i*nChannels + nchannel];
            spectrum[i*2 + 1] = 0; // imagenary part.
        }
        FreqSpectrum(spectrum, sig_len);
    }

    // Generates additive white Gaussian Noise samples with zero mean and a standard deviation of 1.
    // https://www.embeddedrelated.com/showcode/311.php
    double AWGN_generator() {
        const double epsilon = 1.0 / (double)RAND_MAX;
        double temp1;
        double temp2 = epsilon;
        double result;
        int p;

        p = 1;

        while (p > 0) {
            temp2 = ( rand() / ( (double)RAND_MAX ) );

            if ( temp2 <= epsilon ) {
                // temp2 is >= (RAND_MAX / 2)
                p = 1;
            } else {
               p = -1;
            }
        }

        temp1 = cos( ( 2.0 * M_PI ) * rand() / ( (double)RAND_MAX ) );
        result = sqrt( -2.0 * log( temp2 ) ) * temp1;

        return result;    // return the generated random sample to the caller
    }
};

TEST(multichannel_resampler, mute_second_channel) {
    CHECK(resampler->set_scaling(0.5f));
    const size_t sig_len = 2048;
    double buff[sig_len*2];
    size_t i;

    for (size_t n = 0; n < InSamples/nChannels; n++) {
        const packet::sample_t s = (packet::sample_t)AWGN_generator();
        reader.add(1, s);
        for (size_t k = 0; k < nChannels - 1; k++) {
            reader.add(1, s);
        }
    }

    // Put the spectrum of the resampled signal into buff.
    // Odd elements are magnitudes in dB, even elements are phases in radians.
    get_sample_spectrum(buff, sig_len, 0);

    for (i = 0; i < sig_len-1; i += 2) {
        if (i <= sig_len * 0.90 / 2){
            CHECK(buff[i] >= -60);
        } else if (i >= sig_len * 1.00 / 2){
            CHECK(buff[i] <= -60);
        }
    }
}

} // namespace test
} // namespace roc
