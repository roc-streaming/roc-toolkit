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

enum { OutSamples = FrameSize * 100 + 1, InSamples = OutSamples + (FrameSize * 3) };

} // namespace

TEST_GROUP(resampler) {
    TestStreamReader<InSamples> reader;

    core::ScopedPtr<Resampler> resampler;

    void setup() {
        resampler.reset(new Resampler(reader, default_buffer_composer(), FrameSize));
    }

    void expect_buffers(size_t num_buffers, size_t sz, int value) {
        read_buffers<InSamples>(*resampler, num_buffers, sz, value);
    }

    // Reads signal from the resampler and puts its spectrum into @p spectrum.
    //
    // Spectrum must have twice bigger space than the length of the input signal.
    void get_sample_spectrum(double *spectrum, const size_t sig_len) {
        audio::ISampleBufferPtr buf = new_buffer<InSamples>(sig_len);
        resampler->read(*buf);
        LONGS_EQUAL(sig_len, buf->size());
        for (size_t i = 0; i < sig_len; ++i) {
            spectrum[i*2] = (double)buf->data()[i];
            spectrum[i*2 + 1] = 0; // imagenary part.
        }
        FreqSpectrum(spectrum, sig_len);
    }

    // Generates additive white Gaussian Noise samples with zero mean and a standard deviation of 1.
    // https://www.embeddedrelated.com/showcode/311.php
    double AWGN_generator() {
        double temp1;
        double temp2;
        double result;
        int p;

        p = 1;

        while (p > 0) {
            temp2 = ( rand() / ( (double)RAND_MAX ) );

            if ( temp2 == 0 ) {
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

IGNORE_TEST(resampler, invalid_scaling) {
    enum { InvalidScaling = FrameSize };

    CHECK(!resampler->set_scaling(InvalidScaling));
}

IGNORE_TEST(resampler, no_scaling_one_read) {
    CHECK(resampler->set_scaling(1.0f));

    reader.add(InSamples, 333);

    expect_buffers(1, OutSamples, 333);
}

IGNORE_TEST(resampler, no_scaling_multiple_reads) {
    CHECK(resampler->set_scaling(1.0f));

    for (int n = 0; n < InSamples; n++) {
        reader.add(1, n);
    }

    for (int n = 0; n < OutSamples; n++) {
        expect_buffers(1, 1, FrameSize + n);
    }
}

TEST(resampler, upscaling_twice_single) {
    CHECK(resampler->set_scaling(0.5f));
    const size_t sig_len = 2048;
    double buff[sig_len*2];

    for (size_t n = 0; n < InSamples; n++) {
        const packet::sample_t s = (packet::sample_t)sin(M_PI/4 * double(n));
        reader.add(1, s);
    }

    // Put the spectrum of the resampled signal into buff.
    // Odd elements are magnitudes in dB, even elements are phases in radians.
    get_sample_spectrum(buff, sig_len);

    const size_t main_freq_index = sig_len / 8;
    for (size_t n = 0; n < sig_len / 2; n += 2) {
        // The main sinewave frequency decreased twice as we've upsampled.
        // So here SNR is checked.
        CHECK((buff[n] - buff[main_freq_index]) <= -110 || n == main_freq_index);
    }
}

TEST(resampler, upscaling_twice_awgn) {
    CHECK(resampler->set_scaling(2.0f));
    const size_t sig_len = 2048;
    double buff[sig_len*2];
    size_t i;

    FILE *fout = fopen("/tmp/resampler.out", "w+");
    CHECK(fout);

    for (size_t n = 0; n < InSamples; n++) {
        const packet::sample_t s = (packet::sample_t)AWGN_generator();
        reader.add(1, s);
        if (n*2 < ROC_ARRAY_SIZE(buff)) {
            buff[n*2] = s;
            buff[n*2+1] = 0;
            if (n == 0){
                fprintf(fout, "%.16f", s);
            } else {
                fprintf(fout, ", %.16f", s);
            }
        }
    }

    // Put the spectrum of the resampled signal into buff.
    // Odd elements are magnitudes in dB, even elements are phases in radians.
    get_sample_spectrum(buff, sig_len);

    fprintf(fout, "\n");
    for (i = 0; i < ROC_ARRAY_SIZE(buff)/2-1; i += 1) {
        fprintf(fout, "%.16f, ", buff[i]);
    }
    fprintf(fout, "%.16f\n", buff[i]);
    // for (i = 1; i < ROC_ARRAY_SIZE(buff)-1; i += 2) {
    //     fprintf(fout, "%f, ", buff[i]);
    // }
    // fprintf(fout, "%f\n", buff[i]);

    fclose(fout);
}

} // namespace test
} // namespace roc
