/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_reader.h"
#include "test_helpers/mock_writer.h"

#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_reader.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/heap_allocator.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

enum { InFrameSize = 128, OutFrameSize = 200, MaxFrameSize = 4000 };

enum ResamplerMethod { Reader, Writer };

const ResamplerMethod resampler_methods[] = { Reader, Writer };

core::HeapAllocator allocator;
core::BufferPool<sample_t> buffer_pool(allocator, MaxFrameSize, true);

void generate_sine(sample_t* out, size_t num_samples, size_t num_padding) {
    for (size_t n = 0; n < num_samples; n++) {
        out[n] = n < num_padding
            ? 0.0f
            : (sample_t)std::sin(M_PI / 10 * double(n - num_padding)) * 0.8f;
    }
}

void mix_stereo(sample_t* out,
                const sample_t* left,
                const sample_t* right,
                size_t num_samples) {
    for (size_t n = 0; n < num_samples; n++) {
        *out++ = left[n];
        *out++ = right[n];
    }
}

void extract_channel(
    sample_t* out, const sample_t* in, int in_ch, int ch_idx, size_t num_samples) {
    for (size_t n = 0; n < num_samples; n++) {
        *out++ = in[ch_idx];
        in += in_ch;
    }
}

void trim_leading_zeros(sample_t* sig, size_t num_samples, float threshold) {
    size_t n = 0;
    for (; n < num_samples - 1; n++) {
        if (std::abs(sig[n + 2]) >= threshold) {
            break;
        }
    }
    memmove(sig, sig + n, sizeof(sample_t) * (num_samples - n));
}

void truncate(sample_t* sig, size_t num_samples, size_t num_padding) {
    for (size_t n = num_samples - num_padding; n < num_samples; n++) {
        sig[n] = 0.0f;
    }
}

void normalize(sample_t* sig, size_t num_samples) {
    sample_t m = 0;
    for (size_t n = 0; n < num_samples; n++) {
        m = std::max(m, sig[n]);
    }
    for (size_t n = 0; n < num_samples; n++) {
        sig[n] /= m;
    }
}

bool compare(const sample_t* in,
             const sample_t* out,
             size_t num_samples,
             float threshold) {
    for (size_t n = 0; n < num_samples; n++) {
        if (std::abs(in[n] - out[n]) >= threshold) {
            return false;
        }
    }
    return true;
}

void dump(const sample_t* sig1, const sample_t* sig2, size_t num_samples) {
    for (size_t n = 0; n < num_samples; n++) {
        roc_log(LogDebug, "dump %f %f", (double)sig1[n], (double)sig2[n]);
    }
}

void resample_reader(IResampler& resampler,
                     sample_t* in,
                     sample_t* out,
                     size_t num_samples,
                     packet::channel_mask_t channels,
                     size_t sample_rate,
                     float scaling) {
    test::MockReader input_reader;
    for (size_t n = 0; n < num_samples; n++) {
        input_reader.add(1, in[n]);
    }
    input_reader.pad_zeros();

    ResamplerReader rr(input_reader, resampler);
    CHECK(rr.valid());
    CHECK(rr.set_scaling(sample_rate, sample_rate, scaling));

    for (size_t pos = 0; pos < num_samples;) {
        Frame frame(out + pos,
                    std::min(num_samples - pos,
                             (size_t)OutFrameSize * packet::num_channels(channels)));
        CHECK(rr.read(frame));
        pos += frame.size();
    }
}

void resample_writer(IResampler& resampler,
                     sample_t* in,
                     sample_t* out,
                     size_t num_samples,
                     packet::channel_mask_t channels,
                     size_t sample_rate,
                     core::nanoseconds_t frame_duration,
                     float scaling) {
    test::MockWriter output_writer;

    ResamplerWriter rw(output_writer, resampler, buffer_pool, frame_duration, sample_rate,
                       channels);
    CHECK(rw.valid());
    CHECK(rw.set_scaling(sample_rate, sample_rate, scaling));

    for (size_t pos = 0; pos < num_samples;) {
        Frame frame(in + pos,
                    std::min(num_samples - pos,
                             (size_t)OutFrameSize * packet::num_channels(channels)));
        rw.write(frame);
        pos += frame.size();
    }

    for (size_t n = 0; n < num_samples; n++) {
        if (output_writer.num_unread() == 0) {
            break;
        }
        out[n] = output_writer.get();
    }
}

void resample(ResamplerBackend backend,
              ResamplerMethod method,
              sample_t* in,
              sample_t* out,
              size_t num_samples,
              packet::channel_mask_t channels,
              size_t sample_rate,
              float scaling) {
    const core::nanoseconds_t frame_duration = packet::size_to_ns(
        InFrameSize * packet::num_channels(channels), sample_rate, channels);

    core::ScopedPtr<IResampler> resampler(
        ResamplerMap::instance().new_resampler(backend, allocator, buffer_pool,
                                               ResamplerProfile_High, frame_duration,
                                               sample_rate, channels),
        allocator);
    CHECK(resampler);
    CHECK(resampler->valid());

    if (method == Reader) {
        resample_reader(*resampler, in, out, num_samples, channels, sample_rate, scaling);
    } else {
        resample_writer(*resampler, in, out, num_samples, channels, sample_rate,
                        frame_duration, scaling);
    }
}

} // namespace

TEST_GROUP(resampler) {};

TEST(resampler, supported_scalings) {
    enum { ChMask = 0x1, NumIters = 2 };

    ResamplerProfile profiles[] = { ResamplerProfile_Low, ResamplerProfile_Medium,
                                    ResamplerProfile_High };
    size_t frame_sizes[] = { 128, 256, 512 };
    size_t rates[] = { 44100, 48000 };
    float scalings[] = { 0.95f, 0.99f, 1.00f, 1.01f, 1.05f };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);
        for (size_t pn = 0; pn < ROC_ARRAY_SIZE(profiles); pn++) {
            for (size_t fn = 0; fn < ROC_ARRAY_SIZE(frame_sizes); fn++) {
                for (size_t irate = 0; irate < ROC_ARRAY_SIZE(rates); irate++) {
                    for (size_t orate = 0; orate < ROC_ARRAY_SIZE(rates); orate++) {
                        for (size_t sn = 0; sn < ROC_ARRAY_SIZE(scalings); sn++) {
                            core::ScopedPtr<IResampler> resampler(
                                ResamplerMap::instance().new_resampler(
                                    backend, allocator, buffer_pool, profiles[pn],
                                    packet::size_to_ns(frame_sizes[fn], rates[irate],
                                                       ChMask),
                                    rates[irate], ChMask),
                                allocator);
                            CHECK(resampler);
                            CHECK(resampler->valid());

                            test::MockReader input_reader;
                            input_reader.pad_zeros();

                            ResamplerReader rr(input_reader, *resampler);
                            CHECK(rr.valid());

                            for (int iter = 0; iter < NumIters; iter++) {
                                if (!rr.set_scaling(rates[irate], rates[orate],
                                                    scalings[sn])) {
                                    roc_panic("set_scaling() failed:"
                                              " irate=%d orate=%d scaling=%f frame=%d"
                                              " profile=%d backend=%d iteration=%d",
                                              (int)rates[irate], (int)rates[orate],
                                              (double)scalings[sn], (int)frame_sizes[fn],
                                              (int)profiles[pn], (int)backend, iter);
                                }

                                sample_t samples[32];
                                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                                CHECK(rr.read(frame));
                            }
                        }
                    }
                }
            }
        }
    }
}

TEST(resampler, invalid_scalings) {
    enum { SampleRate = 44100, ChMask = 0x1 };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(
                backend, allocator, buffer_pool, ResamplerProfile_High,
                packet::size_to_ns(InFrameSize, SampleRate, ChMask), SampleRate, ChMask),
            allocator);
        CHECK(resampler);
        CHECK(resampler->valid());

        CHECK(!resampler->set_scaling(0, SampleRate, 1.0f));
        CHECK(!resampler->set_scaling(SampleRate, 0, 1.0f));

        CHECK(!resampler->set_scaling(SampleRate, SampleRate, 0.0f));
        CHECK(!resampler->set_scaling(SampleRate, SampleRate, -0.001f));
        CHECK(!resampler->set_scaling(SampleRate, SampleRate, 10000000000.0f));

        CHECK(resampler->set_scaling(SampleRate, SampleRate, 1.0f));
    }
}

TEST(resampler, upscale_downscale_mono) {
    enum {
        SampleRate = 44100,
        ChMask = 0x1,
        NumPad = 2 * OutFrameSize,
        NumTruncate = 8 * OutFrameSize,
        NumSamples = 50 * OutFrameSize
    };

    const float Scaling = 0.97f;
    const float Threshold = 0.06f;

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        for (size_t n_meth = 0; n_meth < ROC_ARRAY_SIZE(resampler_methods); n_meth++) {
            ResamplerMethod method = resampler_methods[n_meth];

            sample_t input[NumSamples];
            generate_sine(input, NumSamples, NumPad);

            sample_t upscaled[NumSamples] = {};
            resample(backend, method, input, upscaled, NumSamples, ChMask, SampleRate,
                     Scaling);

            sample_t downscaled[NumSamples] = {};
            resample(backend, method, upscaled, downscaled, NumSamples, ChMask,
                     SampleRate, 1.0f / Scaling);

            trim_leading_zeros(input, NumSamples, Threshold);
            trim_leading_zeros(upscaled, NumSamples, Threshold);
            trim_leading_zeros(downscaled, NumSamples, Threshold);

            truncate(input, NumSamples, NumTruncate);
            truncate(upscaled, NumSamples, NumTruncate);
            truncate(downscaled, NumSamples, NumTruncate);

            normalize(input, NumSamples);
            normalize(upscaled, NumSamples);
            normalize(downscaled, NumSamples);

            if (compare(input, upscaled, NumSamples, Threshold)) {
                // for plot_resampler_test_dump.py
                dump(input, upscaled, NumSamples);

                roc_panic(
                    "compare with upscaled unexpectedly succeeded: backend=%d method=%d",
                    (int)backend, (int)method);
            }

            if (!compare(input, downscaled, NumSamples, Threshold)) {
                // for plot_resampler_test_dump.py
                dump(input, downscaled, NumSamples);

                roc_panic(
                    "compare with downscaled unexpectedly failed: backend=%d method=%d",
                    (int)backend, (int)method);
            }
        }
    }
}

TEST(resampler, upscale_downscale_stereo) {
    enum {
        SampleRate = 44100,
        NumCh = 2,
        ChMask = 0x3,
        NumPad = 2 * OutFrameSize,
        NumTruncate = 8 * OutFrameSize,
        NumSamples = 50 * OutFrameSize
    };

    const float Scaling = 0.97f;
    const float Threshold = 0.06f;

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        for (size_t n_meth = 0; n_meth < ROC_ARRAY_SIZE(resampler_methods); n_meth++) {
            ResamplerMethod method = resampler_methods[n_meth];

            sample_t input_ch[NumCh][NumSamples];
            generate_sine(input_ch[0], NumSamples, NumPad);
            generate_sine(input_ch[1], NumSamples, NumPad);

            sample_t input[NumSamples * NumCh];
            mix_stereo(input, input_ch[0], input_ch[1], NumSamples);

            sample_t upscaled[NumSamples * NumCh] = {};
            resample(backend, method, input, upscaled, NumSamples * NumCh, ChMask,
                     SampleRate, Scaling);

            sample_t downscaled[NumSamples * NumCh] = {};
            resample(backend, method, upscaled, downscaled, NumSamples * NumCh, ChMask,
                     SampleRate, 1.0f / Scaling);

            for (int ch = 0; ch < NumCh; ch++) {
                sample_t upscaled_ch[NumSamples] = {};
                extract_channel(upscaled_ch, upscaled, NumCh, ch, NumSamples);

                sample_t downscaled_ch[NumSamples] = {};
                extract_channel(downscaled_ch, downscaled, NumCh, ch, NumSamples);

                trim_leading_zeros(input_ch[ch], NumSamples, Threshold);
                trim_leading_zeros(upscaled_ch, NumSamples, Threshold);
                trim_leading_zeros(downscaled_ch, NumSamples, Threshold);

                truncate(input_ch[ch], NumSamples, NumTruncate);
                truncate(upscaled_ch, NumSamples, NumTruncate);
                truncate(downscaled_ch, NumSamples, NumTruncate);

                normalize(input_ch[ch], NumSamples);
                normalize(upscaled_ch, NumSamples);
                normalize(downscaled_ch, NumSamples);

                if (compare(input_ch[ch], upscaled_ch, NumSamples, Threshold)) {
                    // for plot_resampler_test_dump.py
                    dump(input_ch[ch], upscaled_ch, NumSamples);

                    roc_panic("compare with upscaled unexpectedly succeeded:"
                              " backend=%d method=%d",
                              (int)backend, (int)method);
                }

                if (!compare(input_ch[ch], downscaled_ch, NumSamples, Threshold)) {
                    // for plot_resampler_test_dump.py
                    dump(input_ch[ch], downscaled_ch, NumSamples);

                    roc_panic("compare with downscaled unexpectedly failed:"
                              " backend=%d method=%d",
                              (int)backend, (int)method);
                }
            }
        }
    }
}

} // namespace audio
} // namespace roc
