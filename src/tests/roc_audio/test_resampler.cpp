/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "test_helpers/mock_reader.h"
#include "test_helpers/mock_writer.h"

#include "roc_audio/iresampler.h"
#include "roc_audio/resampler_backend.h"
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_reader.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/heap_arena.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

namespace {

enum { InFrameSize = 128, OutFrameSize = 200, MaxFrameSize = 4000 };

enum ResamplerMethod { Reader, Writer };

const ResamplerMethod resampler_methods[] = { Reader, Writer };

core::HeapArena arena;
core::BufferFactory<sample_t> buffer_factory(arena, MaxFrameSize);

class TimestampChecker : public IFrameWriter {
public:
    TimestampChecker(const core::nanoseconds_t capt_ts,
                     const core::nanoseconds_t epsilon,
                     const audio::SampleSpec& sample_spec)
        : capt_ts_(capt_ts)
        , epsilon_(epsilon)
        , sample_spec_(sample_spec)
        , scale_(1.f)
        , start_(true) {
    }

    void set_scaling(const sample_t scale) {
        scale_ = scale;
    }

    virtual void write(Frame& frame) {
        if (capt_ts_ && epsilon_) {
            if (start_) {
                start_ = false;
                CHECK(frame.capture_timestamp() >= capt_ts_);
                capt_ts_ = frame.capture_timestamp();
            } else {
                CHECK(
                    core::ns_equal_delta(capt_ts_, frame.capture_timestamp(), epsilon_));
            }
            capt_ts_ += core::nanoseconds_t(
                sample_spec_.samples_overall_2_ns(frame.num_samples()) * scale_);
        }
    }

private:
    core::nanoseconds_t capt_ts_;
    core::nanoseconds_t epsilon_;
    const audio::SampleSpec& sample_spec_;
    sample_t scale_;

    bool start_;
};

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
                     const SampleSpec& sample_spec,
                     float scaling) {
    test::MockReader input_reader;
    for (size_t n = 0; n < num_samples; n++) {
        input_reader.add_samples(1, in[n]);
    }
    input_reader.add_zero_samples();

    ResamplerReader rr(input_reader, resampler, sample_spec, sample_spec);
    CHECK(rr.is_valid());
    CHECK(rr.set_scaling(scaling));

    for (size_t pos = 0; pos < num_samples;) {
        Frame frame(out + pos,
                    std::min(num_samples - pos,
                             (size_t)OutFrameSize * sample_spec.num_channels()));
        CHECK(rr.read(frame));
        pos += frame.num_samples();
    }
}

void resample_writer(IResampler& resampler,
                     sample_t* in,
                     sample_t* out,
                     size_t num_samples,
                     const SampleSpec& sample_spec,
                     float scaling) {
    test::MockWriter output_writer;

    ResamplerWriter rw(output_writer, resampler, buffer_factory, sample_spec,
                       sample_spec);
    CHECK(rw.is_valid());
    CHECK(rw.set_scaling(scaling));

    for (size_t pos = 0; pos < num_samples;) {
        Frame frame(in + pos,
                    std::min(num_samples - pos,
                             (size_t)OutFrameSize * sample_spec.num_channels()));
        rw.write(frame);
        pos += frame.num_samples();
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
              const SampleSpec& sample_spec,
              float scaling) {
    core::ScopedPtr<IResampler> resampler(
        ResamplerMap::instance().new_resampler(backend, arena, buffer_factory,
                                               ResamplerProfile_High, sample_spec,
                                               sample_spec),
        arena);
    CHECK(resampler);
    CHECK(resampler->is_valid());

    if (method == Reader) {
        resample_reader(*resampler, in, out, num_samples, sample_spec, scaling);
    } else {
        resample_writer(*resampler, in, out, num_samples, sample_spec, scaling);
    }
}

} // namespace

TEST_GROUP(resampler) {};

TEST(resampler, supported_scalings) {
    enum { ChMask = 0x1, NumIters = 2 };

    ResamplerProfile profiles[] = { ResamplerProfile_Low, ResamplerProfile_Medium,
                                    ResamplerProfile_High };
    size_t rates[] = { 8000, 11025, 16000, 22050, 44100, 48000, 88200, 96000 };
    float scalings[] = { 0.99f, 0.999f, 1.000f, 1.001f, 1.01f };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);
        for (size_t pn = 0; pn < ROC_ARRAY_SIZE(profiles); pn++) {
            for (size_t irate = 0; irate < ROC_ARRAY_SIZE(rates); irate++) {
                const SampleSpec in_sample_specs =
                    SampleSpec(rates[irate], ChanLayout_Surround, ChMask);
                for (size_t orate = 0; orate < ROC_ARRAY_SIZE(rates); orate++) {
                    const SampleSpec out_sample_specs =
                        SampleSpec(rates[orate], ChanLayout_Surround, ChMask);
                    // TODO: understand why builtin resampler does not like
                    // when rates differ too much
                    if (backend == ResamplerBackend_Builtin
                        && (float)std::max(irate, orate) / std::min(irate, orate)
                            > 1.1f) {
                        continue;
                    }
                    for (size_t sn = 0; sn < ROC_ARRAY_SIZE(scalings); sn++) {
                        core::ScopedPtr<IResampler> resampler(
                            ResamplerMap::instance().new_resampler(
                                backend, arena, buffer_factory, profiles[pn],
                                in_sample_specs, out_sample_specs),
                            arena);
                        CHECK(resampler);
                        CHECK(resampler->is_valid());

                        test::MockReader input_reader;
                        input_reader.add_zero_samples();

                        ResamplerReader rr(input_reader, *resampler, in_sample_specs,
                                           out_sample_specs);
                        CHECK(rr.is_valid());

                        for (int iter = 0; iter < NumIters; iter++) {
                            if (!rr.set_scaling(scalings[sn])) {
                                roc_panic("set_scaling() failed:"
                                          " irate=%d orate=%d scaling=%f"
                                          " profile=%d backend=%d iteration=%d",
                                          (int)rates[irate], (int)rates[orate],
                                          (double)scalings[sn], (int)profiles[pn],
                                          (int)backend, iter);
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

TEST(resampler, invalid_scalings) {
    enum { SampleRate = 44100, ChMask = 0x1 };
    const SampleSpec SampleSpecs(SampleRate, ChanLayout_Surround, ChMask);

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);
        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(backend, arena, buffer_factory,
                                                   ResamplerProfile_High, SampleSpecs,
                                                   SampleSpecs),
            arena);
        CHECK(resampler);
        CHECK(resampler->is_valid());

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
    const SampleSpec SampleSpecs(SampleRate, ChanLayout_Surround, ChMask);

    const float Scaling = 0.97f;
    const float Threshold = 0.06f;

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);

        for (size_t n_meth = 0; n_meth < ROC_ARRAY_SIZE(resampler_methods); n_meth++) {
            ResamplerMethod method = resampler_methods[n_meth];

            sample_t input[NumSamples];
            generate_sine(input, NumSamples, NumPad);

            sample_t upscaled[NumSamples] = {};
            resample(backend, method, input, upscaled, NumSamples, SampleSpecs, Scaling);

            sample_t downscaled[NumSamples] = {};
            resample(backend, method, upscaled, downscaled, NumSamples, SampleSpecs,
                     1.0f / Scaling);

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
    const SampleSpec SampleSpecs(SampleRate, ChanLayout_Surround, ChMask);

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
            resample(backend, method, input, upscaled, NumSamples * NumCh, SampleSpecs,
                     Scaling);

            sample_t downscaled[NumSamples * NumCh] = {};
            resample(backend, method, upscaled, downscaled, NumSamples * NumCh,
                     SampleSpecs, 1.0f / Scaling);

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

// Testing how resampler deals with timestamps: output frame timestamp must accumulate
// number of previous sammples multiplid by immediate sample rate.
TEST(resampler, timestamp_passthrough_reader) {
    enum {
        InSampleRate = 44100,
        OutSampleRate = 48000,
        NumCh = 2,
        ChMask = 0x3,
        FrameLen = 178
    };
    const SampleSpec InSampleSpecs =
        SampleSpec(InSampleRate, ChanLayout_Surround, ChMask);
    const SampleSpec OutSampleSpecs =
        SampleSpec(OutSampleRate, ChanLayout_Surround, ChMask);

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);
        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(backend, arena, buffer_factory,
                                                   ResamplerProfile_High, InSampleSpecs,
                                                   OutSampleSpecs),
            arena);

        const core::nanoseconds_t start_ts = 1691499037871419405;
        core::nanoseconds_t cur_ts = start_ts;
        core::nanoseconds_t ts_step;

        // Time Stamp allowance. Built in resampler passess the test with 0.1 --
        // much better.
        const core::nanoseconds_t epsilon =
            core::nanoseconds_t(1. / InSampleRate * core::Second);

        test::MockReader input_reader;
        input_reader.enable_timestamps(start_ts, InSampleSpecs);
        input_reader.add_zero_samples();
        ResamplerReader rreader(input_reader, *resampler, InSampleSpecs, OutSampleSpecs);
        // Immediate sample rate.
        float scale = 1.0f;

        CHECK(rreader.set_scaling(scale));
        ts_step =
            core::nanoseconds_t(OutSampleSpecs.samples_overall_2_ns(FrameLen) * scale);

        sample_t samples[FrameLen];

        {
            {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                CHECK(rreader.read(frame));
                CHECK(frame.capture_timestamp() >= start_ts);
                cur_ts = frame.capture_timestamp();
            }
            for (size_t i = 0; i < 10; i++) {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                CHECK(rreader.read(frame));
                cur_ts += ts_step;
                CHECK(core::ns_equal_delta(frame.capture_timestamp(), cur_ts, epsilon));
            }
        }

        scale = 0.95f;
        rreader.set_scaling(scale);
        {
            {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                CHECK(rreader.read(frame));
                cur_ts += ts_step;
                CHECK(core::ns_equal_delta(frame.capture_timestamp(), cur_ts, epsilon));
                ts_step = core::nanoseconds_t(
                    OutSampleSpecs.samples_overall_2_ns(FrameLen) * scale);
            }
            for (size_t i = 0; i < 10; i++) {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                CHECK(rreader.read(frame));
                cur_ts += ts_step;
                CHECK(core::ns_equal_delta(frame.capture_timestamp(), cur_ts, epsilon));
            }
        }

        scale = 1.05f;
        rreader.set_scaling(scale);
        {
            {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                CHECK(rreader.read(frame));
                cur_ts += ts_step;
                CHECK(core::ns_equal_delta(frame.capture_timestamp(), cur_ts, epsilon));
                ts_step = core::nanoseconds_t(
                    OutSampleSpecs.samples_overall_2_ns(FrameLen) * scale);
            }
            for (size_t i = 0; i < 10; i++) {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                CHECK(rreader.read(frame));
                cur_ts += ts_step;
                CHECK(core::ns_equal_delta(frame.capture_timestamp(), cur_ts, epsilon));
            }
        }
    }
}

// Tests resampler writer ability to pass through capture timestamps of frames.
// It copies the method from the same test for resampler reader.
TEST(resampler, timestamp_passthrough_writer) {
    enum {
        InSampleRate = 44100,
        OutSampleRate = 48000,
        NumCh = 2,
        ChMask = 0x3,
        FrameLen = 178
    };
    const SampleSpec InSampleSpecs =
        SampleSpec(InSampleRate, ChanLayout_Surround, ChMask);
    const SampleSpec OutSampleSpecs =
        SampleSpec(OutSampleRate, ChanLayout_Surround, ChMask);

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);
        core::ScopedPtr<IResampler> resampler(
            ResamplerMap::instance().new_resampler(backend, arena, buffer_factory,
                                                   ResamplerProfile_High, InSampleSpecs,
                                                   OutSampleSpecs),
            arena);

        const core::nanoseconds_t start_ts = 1691499037871419405;
        core::nanoseconds_t cur_ts = start_ts;
        core::nanoseconds_t ts_step;

        // Time Stamp allowance. Built in resampler passess the test with 0.1 --
        // much better.
        const core::nanoseconds_t epsilon =
            core::nanoseconds_t(1. / InSampleRate * core::Second);

        TimestampChecker ts_checker(start_ts, epsilon, OutSampleSpecs);

        ResamplerWriter rwriter(ts_checker, *resampler, buffer_factory, InSampleSpecs,
                                OutSampleSpecs);
        // Immediate sample rate.
        float scale = 1.0f;

        CHECK(rwriter.set_scaling(scale));
        ts_step = core::nanoseconds_t(InSampleSpecs.samples_overall_2_ns(FrameLen));

        sample_t samples[FrameLen];
        {
            {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                frame.set_capture_timestamp(cur_ts);
                rwriter.write(frame);
                cur_ts = frame.capture_timestamp();
            }
            for (size_t i = 0; i < 10; i++) {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                cur_ts += ts_step;
                frame.set_capture_timestamp(cur_ts);
                rwriter.write(frame);
            }
        }

        scale = 0.95f;
        rwriter.set_scaling(scale);
        ts_checker.set_scaling(scale);
        {
            {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                cur_ts += ts_step;
                frame.set_capture_timestamp(cur_ts);
                rwriter.write(frame);
            }
            for (size_t i = 0; i < 10; i++) {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                cur_ts += ts_step;
                frame.set_capture_timestamp(cur_ts);
                rwriter.write(frame);
            }
        }

        scale = 1.05f;
        rwriter.set_scaling(scale);
        ts_checker.set_scaling(scale);
        {
            {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                cur_ts += ts_step;
                frame.set_capture_timestamp(cur_ts);
                rwriter.write(frame);
            }
            for (size_t i = 0; i < 10; i++) {
                Frame frame(samples, ROC_ARRAY_SIZE(samples));
                cur_ts += ts_step;
                frame.set_capture_timestamp(cur_ts);
                rwriter.write(frame);
            }
        }
    }
}

} // namespace audio
} // namespace roc
