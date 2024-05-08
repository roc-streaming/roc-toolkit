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
#include "roc_audio/resampler_map.h"
#include "roc_audio/resampler_reader.h"
#include "roc_audio/resampler_writer.h"
#include "roc_core/heap_arena.h"
#include "roc_core/log.h"
#include "roc_core/scoped_ptr.h"

namespace roc {
namespace audio {

namespace {

enum { InFrameSize = 128, OutFrameSize = 200, MaxFrameSize = 4000 };

enum Direction { Dir_Read, Dir_Write };

const Direction supported_dirs[] = { Dir_Read, Dir_Write };

const ResamplerProfile supported_profiles[] = { ResamplerProfile_Low,
                                                ResamplerProfile_Medium,
                                                ResamplerProfile_High };

const size_t supported_rates[] = {
    // FIXME: the following tests fail with full range of rates:
    //  - supported_scalings: builtin resampler panics on some rates
    //  - scaling_trend: fails on some rates
    // 8000, 11025, 16000, 22050, 44100, 48000, 88200, 96000
    44100, 48000
};

const float supported_scalings[] = { 0.99f, 0.999f, 1.000f, 1.001f, 1.01f };

core::HeapArena arena;
FrameFactory frame_factory(arena, MaxFrameSize * sizeof(sample_t));

void expect_capture_timestamp(core::nanoseconds_t expected,
                              core::nanoseconds_t actual,
                              core::nanoseconds_t epsilon) {
    if (!core::ns_equal_delta(expected, actual, epsilon)) {
        char sbuff[256];
        snprintf(sbuff, sizeof(sbuff),
                 "failed comparing capture timestamps:\n"
                 " expected:  %lld\n"
                 " actual:    %lld\n"
                 " delta:     %lld\n"
                 " max_delta: %lld\n",
                 (long long)expected, (long long)actual, (long long)(expected - actual),
                 (long long)epsilon);
        FAIL(sbuff);
    }
}

class TimestampChecker : public IFrameWriter {
public:
    TimestampChecker(const core::nanoseconds_t capt_ts,
                     const core::nanoseconds_t epsilon,
                     const SampleSpec& sample_spec)
        : capt_ts_(capt_ts)
        , last_ts_(0)
        , epsilon_(epsilon)
        , sample_spec_(sample_spec)
        , scale_(1.f)
        , started_(false) {
    }

    void set_scaling(const sample_t scale) {
        scale_ = scale;
    }

    void set_cts(core::nanoseconds_t cts) {
        capt_ts_ = cts;
    }

    core::nanoseconds_t last_cts() {
        return last_ts_;
    }

    virtual void write(Frame& frame) {
        last_ts_ = frame.capture_timestamp();
        if (capt_ts_ && epsilon_) {
            if (!started_ && frame.capture_timestamp() != 0) {
                started_ = true;
                CHECK(frame.capture_timestamp() >= capt_ts_);
                capt_ts_ = frame.capture_timestamp();
            }
            if (started_) {
                expect_capture_timestamp(capt_ts_, frame.capture_timestamp(), epsilon_);
                capt_ts_ += core::nanoseconds_t(
                    sample_spec_.samples_overall_2_ns(frame.num_raw_samples()) * scale_);
            }
        } else {
            CHECK(frame.capture_timestamp() == 0);
        }
    }

private:
    core::nanoseconds_t capt_ts_;
    core::nanoseconds_t last_ts_;
    core::nanoseconds_t epsilon_;
    const SampleSpec& sample_spec_;
    sample_t scale_;
    bool started_;
};

void generate_sine(sample_t* out, size_t num_samples, size_t num_padding) {
    for (size_t n = 0; n < num_samples; n++) {
        out[n] = n < num_padding
            ? 0.0f
            : (sample_t)std::sin(M_PI / 1000 * double(n - num_padding)) * 0.8f;
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
             float threshold_p99,
             float threshold_p100) {
    size_t n99 = 0;
    for (size_t n = 0; n < num_samples; n++) {
        // 100% of samples should satisfy threshold_p100
        if (std::abs(in[n] - out[n]) >= threshold_p100) {
            return false;
        }
        // 99% of samples should satisfy threshold_p99
        if (std::abs(in[n] - out[n]) >= threshold_p99) {
            n99++;
            if (n99 > num_samples * 0.99) {
                return false;
            }
        }
    }
    return true;
}

void dump(const sample_t* sig1, const sample_t* sig2, size_t num_samples) {
    for (size_t n = 0; n < num_samples; n++) {
        roc_log(LogDebug, "dump %f %f %f", (double)sig1[n], (double)sig2[n],
                std::abs((double)sig1[n] - (double)sig2[n]));
    }
}

void fail(const char* fmt, ...) {
    char buf[512] = {};
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf) - 1, fmt, args);
    va_end(args);
    FAIL(buf);
}

const char* dir_to_str(Direction dir) {
    return dir == Dir_Read ? "read" : "write";
}

// returns expected precision of CTS calculations for given backend,
// measured in number of samples per channel
double timestamp_allowance(ResamplerBackend backend) {
    switch (backend) {
    case ResamplerBackend_Builtin:
        return 0.1;
    case ResamplerBackend_Speex:
        return 5;
    case ResamplerBackend_SpeexDec:
        return 2;
    default:
        break;
    }
    FAIL("bad backend");
    return 0;
}

void resample_read(IResampler& resampler,
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
    LONGS_EQUAL(status::StatusOK, rr.init_status());
    CHECK(rr.set_scaling(scaling));

    for (size_t pos = 0; pos < num_samples;) {
        Frame frame(out + pos,
                    std::min(num_samples - pos,
                             (size_t)OutFrameSize * sample_spec.num_channels()));
        CHECK(rr.read(frame));
        pos += frame.num_raw_samples();
    }
}

void resample_write(IResampler& resampler,
                    sample_t* in,
                    sample_t* out,
                    size_t num_samples,
                    const SampleSpec& sample_spec,
                    float scaling) {
    test::MockWriter output_writer;

    ResamplerWriter rw(output_writer, resampler, frame_factory, sample_spec, sample_spec);
    LONGS_EQUAL(status::StatusOK, rw.init_status());
    CHECK(rw.set_scaling(scaling));

    for (size_t pos = 0; pos < num_samples;) {
        Frame frame(in + pos,
                    std::min(num_samples - pos,
                             (size_t)OutFrameSize * sample_spec.num_channels()));
        rw.write(frame);
        pos += frame.num_raw_samples();
    }

    for (size_t n = 0; n < num_samples; n++) {
        if (output_writer.num_unread() == 0) {
            break;
        }
        out[n] = output_writer.get();
    }
}

ResamplerConfig make_config(ResamplerBackend backend, ResamplerProfile profile) {
    ResamplerConfig config;
    config.backend = backend;
    config.profile = profile;

    return config;
}

void resample(ResamplerBackend backend,
              ResamplerProfile profile,
              Direction dir,
              sample_t* in,
              sample_t* out,
              size_t num_samples,
              const SampleSpec& sample_spec,
              float scaling) {
    core::SharedPtr<IResampler> resampler = ResamplerMap::instance().new_resampler(
        arena, frame_factory, make_config(backend, profile), sample_spec, sample_spec);
    CHECK(resampler);
    LONGS_EQUAL(status::StatusOK, resampler->init_status());

    if (dir == Dir_Read) {
        resample_read(*resampler, in, out, num_samples, sample_spec, scaling);
    } else {
        resample_write(*resampler, in, out, num_samples, sample_spec, scaling);
    }
}

} // namespace

TEST_GROUP(resampler) {};

// Check that supported combinations of rates and scaling
// are accepted by resampler.
TEST(resampler, supported_scalings) {
    enum { ChMask = 0x1, NumIterations = 5 };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    for (size_t n_scale = 0; n_scale < ROC_ARRAY_SIZE(supported_scalings);
                         n_scale++) {
                        const ResamplerBackend backend =
                            ResamplerMap::instance().nth_backend(n_back);

                        const SampleSpec in_spec =
                            SampleSpec(supported_rates[n_irate], Sample_RawFormat,
                                       ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                        const SampleSpec out_spec =
                            SampleSpec(supported_rates[n_orate], Sample_RawFormat,
                                       ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                        core::SharedPtr<IResampler> resampler =
                            ResamplerMap::instance().new_resampler(
                                arena, frame_factory,
                                make_config(backend, supported_profiles[n_prof]), in_spec,
                                out_spec);
                        CHECK(resampler);
                        LONGS_EQUAL(status::StatusOK, resampler->init_status());

                        test::MockReader input_reader;
                        input_reader.add_zero_samples();

                        ResamplerReader rr(input_reader, *resampler, in_spec, out_spec);
                        LONGS_EQUAL(status::StatusOK, rr.init_status());

                        for (int n_iter = 0; n_iter < NumIterations; n_iter++) {
                            if (!rr.set_scaling(supported_scalings[n_scale])) {
                                fail("set_scaling() failed:"
                                     " irate=%d orate=%d scaling=%f"
                                     " profile=%d backend=%s iteration=%d",
                                     (int)supported_rates[n_irate],
                                     (int)supported_rates[n_orate],
                                     (double)supported_scalings[n_scale],
                                     (int)supported_profiles[n_prof],
                                     resampler_backend_to_str(backend), n_iter);
                            }

                            // smoke test
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

// Check that unsupported combinations of rates and scaling
// are rejected by resampler.
TEST(resampler, invalid_scalings) {
    enum { ChMask = 0x1 };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend =
                        ResamplerMap::instance().nth_backend(n_back);

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler =
                        ResamplerMap::instance().new_resampler(
                            arena, frame_factory,
                            make_config(backend, supported_profiles[n_prof]), in_spec,
                            out_spec);
                    CHECK(resampler);
                    LONGS_EQUAL(status::StatusOK, resampler->init_status());

                    // bad input rate
                    CHECK(!resampler->set_scaling(0, out_spec.sample_rate(), 1.0f));

                    // bad output rate
                    CHECK(!resampler->set_scaling(in_spec.sample_rate(), 0, 1.0f));

                    // bad multiplier
                    CHECK(!resampler->set_scaling(in_spec.sample_rate(),
                                                  out_spec.sample_rate(), 0.0f));
                    CHECK(!resampler->set_scaling(in_spec.sample_rate(),
                                                  out_spec.sample_rate(), -0.001f));
                    CHECK(!resampler->set_scaling(
                        in_spec.sample_rate(), out_spec.sample_rate(), 10000000000.0f));

                    // all good
                    CHECK(resampler->set_scaling(in_spec.sample_rate(),
                                                 out_spec.sample_rate(), 1.0f));
                }
            }
        }
    }
}

// Set scaling, continously resample, and check that actual
// scaling eventually becomes close to configured scaling.
TEST(resampler, scaling_trend) {
    enum { ChMask = 0x1, WaitSamples = 3000 };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates); n_irate++) {
            for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                 n_orate++) {
                for (size_t n_scale = 0; n_scale < ROC_ARRAY_SIZE(supported_scalings);
                     n_scale++) {
                    const ResamplerBackend backend =
                        ResamplerMap::instance().nth_backend(n_back);

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    const float scaling = supported_scalings[n_scale];

                    core::SharedPtr<IResampler> resampler =
                        ResamplerMap::instance().new_resampler(
                            arena, frame_factory,
                            make_config(backend, ResamplerProfile_Low), in_spec,
                            out_spec);
                    CHECK(resampler);
                    LONGS_EQUAL(status::StatusOK, resampler->init_status());

                    CHECK(resampler->set_scaling(in_spec.sample_rate(),
                                                 out_spec.sample_rate(), scaling));

                    float total_input = 0, total_output = 0;

                    while (total_input < WaitSamples * 2) {
                        sample_t out[OutFrameSize] = {};
                        const size_t n_out =
                            resampler->pop_output(out, ROC_ARRAY_SIZE(out));
                        total_output += n_out;

                        if (n_out < ROC_ARRAY_SIZE(out)) {
                            const size_t n_in = resampler->begin_push_input().size();
                            resampler->end_push_input();
                            total_input += n_in;
                        }

                        if (total_input > WaitSamples) {
                            const float actual_scaling =
                                (total_input - resampler->n_left_to_process())
                                / (total_output / out_spec.sample_rate()
                                   * in_spec.sample_rate());

                            const float scaling_epsilon = 0.01f;

                            if (std::abs(scaling - actual_scaling) > scaling_epsilon) {
                                fail("\nscaling out of bounds:\n"
                                     " irate=%d orate=%d scaling=%f backend=%s\n"
                                     " total_in=%d total_out=%d\n"
                                     " actual_scale=%f expected_scale=%f",
                                     (int)supported_rates[n_irate],
                                     (int)supported_rates[n_orate],
                                     (double)supported_scalings[n_scale],
                                     resampler_backend_to_str(backend), (int)total_input,
                                     (int)total_output, (double)actual_scaling,
                                     (double)scaling);
                            }
                        }
                    }
                }
            }
        }
    }
}

// Upscale samples, downscale back, and compare results.
// (one-channel version)
TEST(resampler, upscale_downscale_mono) {
    enum {
        SampleRate = 44100,
        ChMask = 0x1,
        NumPad = 2 * OutFrameSize,
        NumTruncate = 8 * OutFrameSize,
        NumSamples = 50 * OutFrameSize
    };

    const float Scaling = 0.97f;
    const float Threshold99 = 0.001f; // threshold for 99% of samples
    const float Threshold100 = 0.01f; // threshold for 100% of samples

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        const ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, Sample_RawFormat, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChMask);

        for (size_t n_dir = 0; n_dir < ROC_ARRAY_SIZE(supported_dirs); n_dir++) {
            const Direction dir = supported_dirs[n_dir];

            sample_t input[NumSamples];
            generate_sine(input, NumSamples, NumPad);

            sample_t upscaled[NumSamples] = {};
            resample(backend, profile, dir, input, upscaled, NumSamples, sample_spec,
                     Scaling);

            sample_t downscaled[NumSamples] = {};
            resample(backend, profile, dir, upscaled, downscaled, NumSamples, sample_spec,
                     1.0f / Scaling);

            trim_leading_zeros(input, NumSamples, Threshold99);
            trim_leading_zeros(upscaled, NumSamples, Threshold99);
            trim_leading_zeros(downscaled, NumSamples, Threshold99);

            truncate(input, NumSamples, NumTruncate);
            truncate(upscaled, NumSamples, NumTruncate);
            truncate(downscaled, NumSamples, NumTruncate);

            normalize(input, NumSamples);
            normalize(upscaled, NumSamples);
            normalize(downscaled, NumSamples);

            if (compare(input, upscaled, NumSamples, Threshold99, Threshold100)) {
                // for plot_resampler_test_dump.py
                dump(input, upscaled, NumSamples);

                fail("compare with upscaled unexpectedly succeeded: backend=%s dir=%s",
                     resampler_backend_to_str(backend), dir_to_str(dir));
            }

            if (!compare(input, downscaled, NumSamples, Threshold99, Threshold100)) {
                // for plot_resampler_test_dump.py
                dump(input, downscaled, NumSamples);

                fail("compare with downscaled unexpectedly failed: backend=%s dir=%s",
                     resampler_backend_to_str(backend), dir_to_str(dir));
            }
        }
    }
}

// Upscale samples, downscale back, and compare results.
// (two-channel version)
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
    const float Threshold99 = 0.001f; // threshold for 99% of samples
    const float Threshold100 = 0.01f; // threshold for 100% of samples

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        const ResamplerBackend backend = ResamplerMap::instance().nth_backend(n_back);
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, Sample_RawFormat, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChMask);

        for (size_t n_dir = 0; n_dir < ROC_ARRAY_SIZE(supported_dirs); n_dir++) {
            const Direction dir = supported_dirs[n_dir];

            sample_t input_ch[NumCh][NumSamples];
            generate_sine(input_ch[0], NumSamples, NumPad);
            generate_sine(input_ch[1], NumSamples, NumPad);

            sample_t input[NumSamples * NumCh];
            mix_stereo(input, input_ch[0], input_ch[1], NumSamples);

            sample_t upscaled[NumSamples * NumCh] = {};
            resample(backend, profile, dir, input, upscaled, NumSamples * NumCh,
                     sample_spec, Scaling);

            sample_t downscaled[NumSamples * NumCh] = {};
            resample(backend, profile, dir, upscaled, downscaled, NumSamples * NumCh,
                     sample_spec, 1.0f / Scaling);

            for (int ch = 0; ch < NumCh; ch++) {
                sample_t upscaled_ch[NumSamples] = {};
                extract_channel(upscaled_ch, upscaled, NumCh, ch, NumSamples);

                sample_t downscaled_ch[NumSamples] = {};
                extract_channel(downscaled_ch, downscaled, NumCh, ch, NumSamples);

                trim_leading_zeros(input_ch[ch], NumSamples, Threshold99);
                trim_leading_zeros(upscaled_ch, NumSamples, Threshold99);
                trim_leading_zeros(downscaled_ch, NumSamples, Threshold99);

                truncate(input_ch[ch], NumSamples, NumTruncate);
                truncate(upscaled_ch, NumSamples, NumTruncate);
                truncate(downscaled_ch, NumSamples, NumTruncate);

                normalize(input_ch[ch], NumSamples);
                normalize(upscaled_ch, NumSamples);
                normalize(downscaled_ch, NumSamples);

                if (compare(input_ch[ch], upscaled_ch, NumSamples, Threshold99,
                            Threshold100)) {
                    // for plot_resampler_test_dump.py
                    dump(input_ch[ch], upscaled_ch, NumSamples);

                    fail("compare with upscaled unexpectedly succeeded:"
                         " backend=%s dir=%s",
                         resampler_backend_to_str(backend), dir_to_str(dir));
                }

                if (!compare(input_ch[ch], downscaled_ch, NumSamples, Threshold99,
                             Threshold100)) {
                    // for plot_resampler_test_dump.py
                    dump(input_ch[ch], downscaled_ch, NumSamples);

                    fail("compare with downscaled unexpectedly failed:"
                         " backend=%s dir=%s",
                         resampler_backend_to_str(backend), dir_to_str(dir));
                }
            }
        }
    }
}

// Testing how resampler deals with timestamps: output frame timestamp must accumulate
// number of previous sammples multiplid by immediate sample rate.
TEST(resampler, reader_timestamp_passthrough) {
    enum { NumCh = 2, ChMask = 0x3, FrameLen = 178, NumIterations = 20 };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend =
                        ResamplerMap::instance().nth_backend(n_back);

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler =
                        ResamplerMap::instance().new_resampler(
                            arena, frame_factory,
                            make_config(backend, supported_profiles[n_prof]), in_spec,
                            out_spec);

                    const core::nanoseconds_t start_ts = 1691499037871419405;
                    core::nanoseconds_t cur_ts = start_ts;
                    core::nanoseconds_t ts_step = 0;

                    const core::nanoseconds_t epsilon =
                        core::nanoseconds_t(1. / in_spec.sample_rate() * core::Second
                                            * timestamp_allowance(backend));

                    test::MockReader input_reader;
                    input_reader.enable_timestamps(start_ts, in_spec);
                    input_reader.add_zero_samples();
                    ResamplerReader rreader(input_reader, *resampler, in_spec, out_spec);
                    // Immediate sample rate.
                    float scale = 1.0f;

                    CHECK(rreader.set_scaling(scale));
                    ts_step = core::nanoseconds_t(out_spec.samples_overall_2_ns(FrameLen)
                                                  * scale);

                    sample_t samples[FrameLen] = {};

                    {
                        {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            CHECK(rreader.read(frame));
                            // Since CTS is estimated based scaling, it can happen
                            // to be in past relative to the very first frame, but only
                            // within allowed epsilon.
                            CHECK(frame.capture_timestamp() >= start_ts - epsilon);
                            cur_ts = frame.capture_timestamp();
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            CHECK(rreader.read(frame));
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame.capture_timestamp(),
                                                     epsilon);
                        }
                    }

                    // Change scaling.
                    scale = 0.95f;
                    rreader.set_scaling(scale);
                    {
                        {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            CHECK(rreader.read(frame));
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame.capture_timestamp(),
                                                     epsilon);
                            ts_step = core::nanoseconds_t(
                                out_spec.samples_overall_2_ns(FrameLen) * scale);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            CHECK(rreader.read(frame));
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame.capture_timestamp(),
                                                     epsilon);
                        }
                    }

                    // Change scaling.
                    scale = 1.05f;
                    rreader.set_scaling(scale);
                    {
                        {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            CHECK(rreader.read(frame));
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame.capture_timestamp(),
                                                     epsilon);
                            ts_step = core::nanoseconds_t(
                                out_spec.samples_overall_2_ns(FrameLen) * scale);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            CHECK(rreader.read(frame));
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame.capture_timestamp(),
                                                     epsilon);
                        }
                    }
                }
            }
        }
    }
}

// Tests resampler writer ability to pass through capture timestamps of frames.
// It copies the method from the same test for resampler reader.
TEST(resampler, writer_timestamp_passthrough) {
    enum {
        NumCh = 2,
        ChMask = 0x3,
        FrameLen = 178,
        NumIterations = 20,
        MaxZeroCtsFrames = 3
    };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend =
                        ResamplerMap::instance().nth_backend(n_back);

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler =
                        ResamplerMap::instance().new_resampler(
                            arena, frame_factory,
                            make_config(backend, supported_profiles[n_prof]), in_spec,
                            out_spec);

                    const core::nanoseconds_t start_ts = 1691499037871419405;
                    core::nanoseconds_t cur_ts = start_ts;
                    core::nanoseconds_t ts_step = 0;

                    const core::nanoseconds_t epsilon =
                        core::nanoseconds_t(1. / in_spec.sample_rate() * core::Second
                                            * timestamp_allowance(backend));

                    TimestampChecker ts_checker(start_ts, epsilon, out_spec);

                    ResamplerWriter rwriter(ts_checker, *resampler, frame_factory,
                                            in_spec, out_spec);
                    // Immediate sample rate.
                    float scale = 1.0f;

                    CHECK(rwriter.set_scaling(scale));
                    ts_step = core::nanoseconds_t(in_spec.samples_overall_2_ns(FrameLen));

                    sample_t samples[FrameLen] = {};

                    {
                        {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            frame.set_capture_timestamp(cur_ts);
                            rwriter.write(frame);
                            cur_ts = frame.capture_timestamp();
                            CHECK(ts_checker.last_cts() >= 0);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            cur_ts += ts_step;
                            frame.set_capture_timestamp(cur_ts);
                            rwriter.write(frame);
                            CHECK(ts_checker.last_cts() >= 0);
                            if (i >= MaxZeroCtsFrames) {
                                CHECK(ts_checker.last_cts() > 0);
                            }
                        }
                    }

                    // Change scaling.
                    scale = 0.95f;
                    rwriter.set_scaling(scale);
                    ts_checker.set_scaling(scale);
                    {
                        {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            cur_ts += ts_step;
                            frame.set_capture_timestamp(cur_ts);
                            rwriter.write(frame);
                            CHECK(ts_checker.last_cts() > 0);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            cur_ts += ts_step;
                            frame.set_capture_timestamp(cur_ts);
                            rwriter.write(frame);
                            CHECK(ts_checker.last_cts() > 0);
                        }
                    }

                    // Change scaling.
                    scale = 1.05f;
                    rwriter.set_scaling(scale);
                    ts_checker.set_scaling(scale);
                    {
                        {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            cur_ts += ts_step;
                            frame.set_capture_timestamp(cur_ts);
                            rwriter.write(frame);
                            CHECK(ts_checker.last_cts() > 0);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            Frame frame(samples, ROC_ARRAY_SIZE(samples));
                            cur_ts += ts_step;
                            frame.set_capture_timestamp(cur_ts);
                            rwriter.write(frame);
                            CHECK(ts_checker.last_cts() > 0);
                        }
                    }
                }
            }
        }
    }
}

// Tests how ResamplerReader handles the case when CTS is at first zero and then
// becomes non-zero, but starts with small value close to Unix Epoch. It should
// never produce negative CTS and return zero CTS instead.
TEST(resampler, reader_timestamp_zero_or_small) {
    enum {
        NumCh = 2,
        ChMask = 0x3,
        FrameLen = 178,
        SmallCts = 5, // close to unix epoch
        NumIterations = 20
    };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend =
                        ResamplerMap::instance().nth_backend(n_back);

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler =
                        ResamplerMap::instance().new_resampler(
                            arena, frame_factory,
                            make_config(backend, supported_profiles[n_prof]), in_spec,
                            out_spec);

                    test::MockReader input_reader;
                    input_reader.add_zero_samples();

                    ResamplerReader rreader(input_reader, *resampler, in_spec, out_spec);

                    // Set scaling.
                    float scale = 1.05f;
                    rreader.set_scaling(scale);

                    // At first, cts is zero.
                    for (size_t i = 0; i < NumIterations; i++) {
                        sample_t samples[FrameLen] = {};
                        Frame frame(samples, ROC_ARRAY_SIZE(samples));
                        CHECK(rreader.read(frame));
                        CHECK(frame.capture_timestamp() == 0);
                    }

                    // Then we switch to non-zero (but very small) cts.
                    const core::nanoseconds_t start_ts = SmallCts;
                    core::nanoseconds_t cur_ts = 0;
                    core::nanoseconds_t ts_step = core::nanoseconds_t(
                        out_spec.samples_overall_2_ns(FrameLen) * scale);

                    const core::nanoseconds_t epsilon =
                        core::nanoseconds_t(1. / in_spec.sample_rate() * core::Second
                                            * timestamp_allowance(backend));

                    input_reader.enable_timestamps(start_ts, in_spec);

                    for (size_t i = 0; i < NumIterations; i++) {
                        sample_t samples[FrameLen] = {};
                        Frame frame(samples, ROC_ARRAY_SIZE(samples));
                        CHECK(rreader.read(frame));
                        if (cur_ts == 0) {
                            if (frame.capture_timestamp() != 0) {
                                cur_ts = frame.capture_timestamp();
                                CHECK(cur_ts >= start_ts - epsilon);
                                CHECK(cur_ts <= start_ts + ts_step);
                            }
                        } else {
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame.capture_timestamp(),
                                                     epsilon);
                        }
                    }
                }
            }
        }
    }
}

// Same as previous test, but for writer.
TEST(resampler, writer_timestamp_zero_or_small) {
    enum {
        NumCh = 2,
        ChMask = 0x3,
        FrameLen = 178,
        SmallCts = 5, // close to unix epoch
        NumIterations = 20,
        MaxZeroCtsFrames = 3,
    };

    for (size_t n_back = 0; n_back < ResamplerMap::instance().num_backends(); n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend =
                        ResamplerMap::instance().nth_backend(n_back);

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], Sample_RawFormat,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler =
                        ResamplerMap::instance().new_resampler(
                            arena, frame_factory,
                            make_config(backend, supported_profiles[n_prof]), in_spec,
                            out_spec);

                    const core::nanoseconds_t epsilon =
                        core::nanoseconds_t(1. / in_spec.sample_rate() * core::Second
                                            * timestamp_allowance(backend));

                    TimestampChecker ts_checker(0, epsilon, out_spec);

                    ResamplerWriter rwriter(ts_checker, *resampler, frame_factory,
                                            in_spec, out_spec);

                    // Set scaling.
                    float scale = 1.05f;
                    ts_checker.set_scaling(scale);
                    rwriter.set_scaling(scale);

                    // At first, cts is zero.
                    for (size_t i = 0; i < NumIterations; i++) {
                        sample_t samples[FrameLen] = {};
                        Frame frame(samples, ROC_ARRAY_SIZE(samples));
                        frame.set_capture_timestamp(0);
                        rwriter.write(frame);
                        CHECK(ts_checker.last_cts() == 0);
                    }

                    // Then we switch to non-zero (but very small) cts.
                    const core::nanoseconds_t start_ts = SmallCts;
                    core::nanoseconds_t cur_ts = start_ts;
                    core::nanoseconds_t ts_step = in_spec.samples_overall_2_ns(FrameLen);

                    ts_checker.set_cts(start_ts);

                    for (size_t i = 0; i < NumIterations; i++) {
                        sample_t samples[FrameLen] = {};
                        Frame frame(samples, ROC_ARRAY_SIZE(samples));
                        frame.set_capture_timestamp(cur_ts);
                        cur_ts += ts_step;
                        rwriter.write(frame);
                        CHECK(ts_checker.last_cts() >= 0);
                        if (i >= MaxZeroCtsFrames) {
                            CHECK(ts_checker.last_cts() > 0);
                        }
                    }
                }
            }
        }
    }
}

} // namespace audio
} // namespace roc
