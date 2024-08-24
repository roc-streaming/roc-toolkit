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
#include "roc_audio/processor_map.h"
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
ProcessorMap processor_map(arena);

FramePtr new_frame(const SampleSpec& sample_spec,
                   size_t n_samples,
                   unsigned flags,
                   core::nanoseconds_t capt_ts) {
    CHECK(n_samples % sample_spec.num_channels() == 0);

    FramePtr frame = frame_factory.allocate_frame(n_samples * sizeof(sample_t));
    CHECK(frame);

    frame->set_raw(true);
    frame->set_flags(flags);
    frame->set_duration(n_samples / sample_spec.num_channels());
    frame->set_capture_timestamp(capt_ts);

    UNSIGNED_LONGS_EQUAL(n_samples, frame->num_raw_samples());

    return frame;
}

void write_frame(IFrameWriter& writer, Frame& frame) {
    LONGS_EQUAL(status::StatusOK, writer.write(frame));
}

void check_frame(Frame& frame, const SampleSpec& sample_spec, size_t n_samples) {
    CHECK(frame.is_raw());

    CHECK(frame.raw_samples());
    CHECK(frame.bytes());

    UNSIGNED_LONGS_EQUAL(n_samples / sample_spec.num_channels(), frame.duration());
    UNSIGNED_LONGS_EQUAL(n_samples, frame.num_raw_samples());
    UNSIGNED_LONGS_EQUAL(n_samples * sizeof(sample_t), frame.num_bytes());
}

FramePtr
read_frame(IFrameReader& reader, const SampleSpec& sample_spec, size_t n_samples) {
    CHECK(n_samples % sample_spec.num_channels() == 0);

    FramePtr frame = frame_factory.allocate_frame_no_buffer();
    CHECK(frame);

    LONGS_EQUAL(status::StatusOK,
                reader.read(*frame, n_samples / sample_spec.num_channels(), ModeHard));

    check_frame(*frame, sample_spec, n_samples);

    return frame;
}

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

    virtual status::StatusCode write(Frame& frame) {
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
        return status::StatusOK;
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

ResamplerConfig make_config(ResamplerBackend backend, ResamplerProfile profile) {
    ResamplerConfig config;
    config.backend = backend;
    config.profile = profile;

    return config;
}

void resample_read(IResampler& resampler,
                   sample_t* in,
                   sample_t* out,
                   size_t num_samples,
                   const SampleSpec& sample_spec,
                   float scaling) {
    test::MockReader input_reader(frame_factory, sample_spec);
    for (size_t n = 0; n < num_samples; n++) {
        input_reader.add_samples(1, in[n]);
    }
    input_reader.add_zero_samples();

    ResamplerReader rreader(input_reader, frame_factory, resampler, sample_spec,
                            sample_spec);
    LONGS_EQUAL(status::StatusOK, rreader.init_status());
    CHECK(rreader.set_scaling(scaling));

    for (size_t pos = 0; pos < num_samples;) {
        const size_t n_samples = std::min(
            num_samples - pos, (size_t)OutFrameSize * sample_spec.num_channels());

        FramePtr frame = read_frame(rreader, sample_spec, n_samples);
        memcpy(out + pos, frame->raw_samples(), n_samples * sizeof(sample_t));
        pos += n_samples;
    }
}

void resample_write(IResampler& resampler,
                    sample_t* in,
                    sample_t* out,
                    size_t num_samples,
                    const SampleSpec& sample_spec,
                    float scaling) {
    test::MockWriter output_writer;

    ResamplerWriter rwriter(output_writer, frame_factory, resampler, sample_spec,
                            sample_spec);
    LONGS_EQUAL(status::StatusOK, rwriter.init_status());
    CHECK(rwriter.set_scaling(scaling));

    for (size_t pos = 0; pos < num_samples;) {
        const size_t n_samples = std::min(
            num_samples - pos, (size_t)OutFrameSize * sample_spec.num_channels());

        FramePtr frame = new_frame(sample_spec, n_samples, 0, 0);
        memcpy(frame->raw_samples(), in + pos, n_samples * sizeof(sample_t));
        write_frame(rwriter, *frame);
        pos += n_samples;
    }

    for (size_t n = 0; n < num_samples; n++) {
        if (output_writer.num_unread() == 0) {
            break;
        }
        out[n] = output_writer.get();
    }
}

void resample(ResamplerBackend backend,
              ResamplerProfile profile,
              Direction dir,
              sample_t* in,
              sample_t* out,
              size_t num_samples,
              const SampleSpec& sample_spec,
              float scaling) {
    core::SharedPtr<IResampler> resampler = processor_map.new_resampler(
        make_config(backend, profile), sample_spec, sample_spec, frame_factory, arena);
    CHECK(resampler);
    LONGS_EQUAL(status::StatusOK, resampler->init_status());

    if (dir == Dir_Read) {
        resample_read(*resampler, in, out, num_samples, sample_spec, scaling);
    } else {
        resample_write(*resampler, in, out, num_samples, sample_spec, scaling);
    }
}

} // namespace

TEST_GROUP(resampler) {
    ResamplerBackend supported_backends[ResamplerBackend_Max];
    size_t n_supported_backends;

    void setup() {
        n_supported_backends = 0;

        for (int n = 0; n < ResamplerBackend_Max; n++) {
            const ResamplerBackend backend = (ResamplerBackend)n;
            if (backend == ResamplerBackend_Auto) {
                continue;
            }
            if (!processor_map.has_resampler_backend(backend)) {
                continue;
            }

            supported_backends[n_supported_backends++] = backend;
        }
    }
};

// Check that supported combinations of rates and scaling
// are accepted by resampler.
TEST(resampler, supported_scalings) {
    enum { ChMask = 0x1, NumIterations = 5 };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    for (size_t n_scale = 0; n_scale < ROC_ARRAY_SIZE(supported_scalings);
                         n_scale++) {
                        const ResamplerBackend backend = supported_backends[n_back];

                        const SampleSpec in_spec =
                            SampleSpec(supported_rates[n_irate], PcmSubformat_Raw,
                                       ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                        const SampleSpec out_spec =
                            SampleSpec(supported_rates[n_orate], PcmSubformat_Raw,
                                       ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                        core::SharedPtr<IResampler> resampler =
                            processor_map.new_resampler(
                                make_config(backend, supported_profiles[n_prof]), in_spec,
                                out_spec, frame_factory, arena);
                        CHECK(resampler);
                        LONGS_EQUAL(status::StatusOK, resampler->init_status());

                        test::MockReader input_reader(frame_factory, in_spec);
                        input_reader.add_zero_samples();

                        ResamplerReader rreader(input_reader, frame_factory, *resampler,
                                                in_spec, out_spec);
                        LONGS_EQUAL(status::StatusOK, rreader.init_status());

                        for (int n_iter = 0; n_iter < NumIterations; n_iter++) {
                            if (!rreader.set_scaling(supported_scalings[n_scale])) {
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
                            FramePtr frame = read_frame(rreader, out_spec, 32);
                            CHECK(frame);
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

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend = supported_backends[n_back];

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler = processor_map.new_resampler(
                        make_config(backend, supported_profiles[n_prof]), in_spec,
                        out_spec, frame_factory, arena);
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

// Set scaling, continuously resample, and check that actual
// scaling eventually becomes close to configured scaling.
TEST(resampler, scaling_trend) {
    enum { ChMask = 0x1, WaitSamples = 3000 };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates); n_irate++) {
            for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                 n_orate++) {
                for (size_t n_scale = 0; n_scale < ROC_ARRAY_SIZE(supported_scalings);
                     n_scale++) {
                    const ResamplerBackend backend = supported_backends[n_back];

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    const float scaling = supported_scalings[n_scale];

                    core::SharedPtr<IResampler> resampler = processor_map.new_resampler(
                        make_config(backend, ResamplerProfile_Low), in_spec, out_spec,
                        frame_factory, arena);
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

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        const ResamplerBackend backend = supported_backends[n_back];
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
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

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        const ResamplerBackend backend = supported_backends[n_back];
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
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
// number of previous samples multiplied by immediate sample rate.
TEST(resampler, reader_timestamp_passthrough) {
    enum { NumCh = 2, ChMask = 0x3, FrameLen = 178, NumIterations = 20 };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend = supported_backends[n_back];

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler = processor_map.new_resampler(
                        make_config(backend, supported_profiles[n_prof]), in_spec,
                        out_spec, frame_factory, arena);

                    const core::nanoseconds_t start_ts = 1691499037871419405;
                    core::nanoseconds_t cur_ts = start_ts;
                    core::nanoseconds_t ts_step = 0;

                    const core::nanoseconds_t epsilon =
                        core::nanoseconds_t(1. / in_spec.sample_rate() * core::Second
                                            * timestamp_allowance(backend));

                    test::MockReader input_reader(frame_factory, in_spec);
                    input_reader.enable_timestamps(start_ts);
                    input_reader.add_zero_samples();
                    ResamplerReader rreader(input_reader, frame_factory, *resampler,
                                            in_spec, out_spec);
                    // Immediate sample rate.
                    float scale = 1.0f;

                    CHECK(rreader.set_scaling(scale));
                    ts_step = core::nanoseconds_t(out_spec.samples_overall_2_ns(FrameLen)
                                                  * scale);

                    {
                        {
                            FramePtr frame = read_frame(rreader, out_spec, FrameLen);
                            // Since CTS is estimated based scaling, it can happen
                            // to be in past relative to the very first frame, but only
                            // within allowed epsilon.
                            CHECK(frame->capture_timestamp() >= start_ts - epsilon);
                            cur_ts = frame->capture_timestamp();
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            FramePtr frame = read_frame(rreader, out_spec, FrameLen);
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame->capture_timestamp(),
                                                     epsilon);
                        }
                    }

                    // Change scaling.
                    scale = 0.95f;
                    rreader.set_scaling(scale);
                    {
                        {
                            FramePtr frame = read_frame(rreader, out_spec, FrameLen);
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame->capture_timestamp(),
                                                     epsilon);
                            ts_step = core::nanoseconds_t(
                                out_spec.samples_overall_2_ns(FrameLen) * scale);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            FramePtr frame = read_frame(rreader, out_spec, FrameLen);
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame->capture_timestamp(),
                                                     epsilon);
                        }
                    }

                    // Change scaling.
                    scale = 1.05f;
                    rreader.set_scaling(scale);
                    {
                        {
                            FramePtr frame = read_frame(rreader, out_spec, FrameLen);
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame->capture_timestamp(),
                                                     epsilon);
                            ts_step = core::nanoseconds_t(
                                out_spec.samples_overall_2_ns(FrameLen) * scale);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            FramePtr frame = read_frame(rreader, out_spec, FrameLen);
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame->capture_timestamp(),
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

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend = supported_backends[n_back];

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler = processor_map.new_resampler(
                        make_config(backend, supported_profiles[n_prof]), in_spec,
                        out_spec, frame_factory, arena);

                    const core::nanoseconds_t start_ts = 1691499037871419405;
                    core::nanoseconds_t cur_ts = start_ts;
                    core::nanoseconds_t ts_step = 0;

                    const core::nanoseconds_t epsilon =
                        core::nanoseconds_t(1. / in_spec.sample_rate() * core::Second
                                            * timestamp_allowance(backend));

                    TimestampChecker ts_checker(start_ts, epsilon, out_spec);

                    ResamplerWriter rwriter(ts_checker, frame_factory, *resampler,
                                            in_spec, out_spec);
                    // Immediate sample rate.
                    float scale = 1.0f;

                    CHECK(rwriter.set_scaling(scale));
                    ts_step = core::nanoseconds_t(in_spec.samples_overall_2_ns(FrameLen));

                    {
                        {
                            FramePtr frame = new_frame(in_spec, FrameLen, 0, cur_ts);
                            write_frame(rwriter, *frame);
                            cur_ts = frame->capture_timestamp();
                            CHECK(ts_checker.last_cts() >= 0);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            cur_ts += ts_step;
                            FramePtr frame = new_frame(in_spec, FrameLen, 0, cur_ts);
                            write_frame(rwriter, *frame);
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
                            cur_ts += ts_step;
                            FramePtr frame = new_frame(in_spec, FrameLen, 0, cur_ts);
                            write_frame(rwriter, *frame);
                            CHECK(ts_checker.last_cts() > 0);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            cur_ts += ts_step;
                            FramePtr frame = new_frame(in_spec, FrameLen, 0, cur_ts);
                            write_frame(rwriter, *frame);
                            CHECK(ts_checker.last_cts() > 0);
                        }
                    }

                    // Change scaling.
                    scale = 1.05f;
                    rwriter.set_scaling(scale);
                    ts_checker.set_scaling(scale);
                    {
                        {
                            cur_ts += ts_step;
                            FramePtr frame = new_frame(in_spec, FrameLen, 0, cur_ts);
                            write_frame(rwriter, *frame);
                            CHECK(ts_checker.last_cts() > 0);
                        }
                        for (size_t i = 0; i < NumIterations; i++) {
                            cur_ts += ts_step;
                            FramePtr frame = new_frame(in_spec, FrameLen, 0, cur_ts);
                            write_frame(rwriter, *frame);
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

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend = supported_backends[n_back];

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler = processor_map.new_resampler(
                        make_config(backend, supported_profiles[n_prof]), in_spec,
                        out_spec, frame_factory, arena);

                    test::MockReader input_reader(frame_factory, in_spec);
                    input_reader.add_zero_samples();

                    ResamplerReader rreader(input_reader, frame_factory, *resampler,
                                            in_spec, out_spec);

                    // Set scaling.
                    float scale = 1.05f;
                    rreader.set_scaling(scale);

                    // At first, cts is zero.
                    for (size_t i = 0; i < NumIterations; i++) {
                        FramePtr frame = read_frame(rreader, out_spec, FrameLen);
                        CHECK(frame->capture_timestamp() == 0);
                    }

                    // Then we switch to non-zero (but very small) cts.
                    const core::nanoseconds_t start_ts = SmallCts;
                    core::nanoseconds_t cur_ts = 0;
                    core::nanoseconds_t ts_step = core::nanoseconds_t(
                        out_spec.samples_overall_2_ns(FrameLen) * scale);

                    const core::nanoseconds_t epsilon =
                        core::nanoseconds_t(1. / in_spec.sample_rate() * core::Second
                                            * timestamp_allowance(backend));

                    input_reader.enable_timestamps(start_ts);

                    for (size_t i = 0; i < NumIterations; i++) {
                        FramePtr frame = read_frame(rreader, out_spec, FrameLen);
                        if (cur_ts == 0) {
                            if (frame->capture_timestamp() != 0) {
                                cur_ts = frame->capture_timestamp();
                                CHECK(cur_ts >= start_ts - epsilon);
                                CHECK(cur_ts <= start_ts + ts_step);
                            }
                        } else {
                            cur_ts += ts_step;
                            expect_capture_timestamp(cur_ts, frame->capture_timestamp(),
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

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        for (size_t n_prof = 0; n_prof < ROC_ARRAY_SIZE(supported_profiles); n_prof++) {
            for (size_t n_irate = 0; n_irate < ROC_ARRAY_SIZE(supported_rates);
                 n_irate++) {
                for (size_t n_orate = 0; n_orate < ROC_ARRAY_SIZE(supported_rates);
                     n_orate++) {
                    const ResamplerBackend backend = supported_backends[n_back];

                    const SampleSpec in_spec =
                        SampleSpec(supported_rates[n_irate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);
                    const SampleSpec out_spec =
                        SampleSpec(supported_rates[n_orate], PcmSubformat_Raw,
                                   ChanLayout_Surround, ChanOrder_Smpte, ChMask);

                    core::SharedPtr<IResampler> resampler = processor_map.new_resampler(
                        make_config(backend, supported_profiles[n_prof]), in_spec,
                        out_spec, frame_factory, arena);

                    const core::nanoseconds_t epsilon =
                        core::nanoseconds_t(1. / in_spec.sample_rate() * core::Second
                                            * timestamp_allowance(backend));

                    TimestampChecker ts_checker(0, epsilon, out_spec);

                    ResamplerWriter rwriter(ts_checker, frame_factory, *resampler,
                                            in_spec, out_spec);

                    // Set scaling.
                    float scale = 1.05f;
                    ts_checker.set_scaling(scale);
                    rwriter.set_scaling(scale);

                    // At first, cts is zero.
                    for (size_t i = 0; i < NumIterations; i++) {
                        FramePtr frame = new_frame(in_spec, FrameLen, 0, 0);
                        write_frame(rwriter, *frame);
                        CHECK(ts_checker.last_cts() == 0);
                    }

                    // Then we switch to non-zero (but very small) cts.
                    const core::nanoseconds_t start_ts = SmallCts;
                    core::nanoseconds_t cur_ts = start_ts;
                    core::nanoseconds_t ts_step = in_spec.samples_overall_2_ns(FrameLen);

                    ts_checker.set_cts(start_ts);

                    for (size_t i = 0; i < NumIterations; i++) {
                        FramePtr frame = new_frame(in_spec, FrameLen, 0, cur_ts);
                        write_frame(rwriter, *frame);
                        cur_ts += ts_step;
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

// When requested frame is big, resampler reader should return partial read.
TEST(resampler, reader_big_frame) {
    enum {
        SampleRate = 44100,
        NumCh = 2,
        ChMask = 0x3,
    };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        const ResamplerBackend backend = supported_backends[n_back];
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChMask);

        test::MockReader input_reader(frame_factory, sample_spec);
        input_reader.add_zero_samples();

        core::SharedPtr<IResampler> resampler =
            processor_map.new_resampler(make_config(backend, profile), sample_spec,
                                        sample_spec, frame_factory, arena);
        CHECK(resampler);
        LONGS_EQUAL(status::StatusOK, resampler->init_status());

        ResamplerReader rreader(input_reader, frame_factory, *resampler, sample_spec,
                                sample_spec);
        LONGS_EQUAL(status::StatusOK, rreader.init_status());

        FramePtr frame = frame_factory.allocate_frame(0);
        CHECK(frame);

        LONGS_EQUAL(status::StatusPart,
                    rreader.read(*frame, MaxFrameSize * 3 / NumCh, ModeHard));

        check_frame(*frame, sample_spec, MaxFrameSize);
    }
}

// When provided frame is big, resampler writer should generate multiple writes.
TEST(resampler, writer_big_frame) {
    enum {
        SampleRate = 44100,
        NumCh = 2,
        ChMask = 0x3,
        Factor = 10,
    };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        const ResamplerBackend backend = supported_backends[n_back];
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChMask);

        test::MockWriter output_writer;

        core::SharedPtr<IResampler> resampler =
            processor_map.new_resampler(make_config(backend, profile), sample_spec,
                                        sample_spec, frame_factory, arena);
        CHECK(resampler);
        LONGS_EQUAL(status::StatusOK, resampler->init_status());

        ResamplerWriter rwriter(output_writer, frame_factory, *resampler, sample_spec,
                                sample_spec);
        LONGS_EQUAL(status::StatusOK, rwriter.init_status());

        FrameFactory big_factory(arena, MaxFrameSize * Factor * sizeof(sample_t));

        FramePtr frame =
            big_factory.allocate_frame(MaxFrameSize * Factor * sizeof(sample_t));
        CHECK(frame);

        frame->set_raw(true);
        frame->set_duration(MaxFrameSize * Factor / NumCh);

        LONGS_EQUAL(status::StatusOK, rwriter.write(*frame));

        CHECK(output_writer.written_samples() > MaxFrameSize * (Factor - 1));
    }
}

// Forward mode to underlying reader.
TEST(resampler, reader_forward_mode) {
    enum {
        SampleRate = 44100,
        NumCh = 2,
        ChMask = 0x3,
    };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        const ResamplerBackend backend = supported_backends[n_back];
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChMask);

        test::MockReader input_reader(frame_factory, sample_spec);
        input_reader.add_zero_samples();

        core::SharedPtr<IResampler> resampler =
            processor_map.new_resampler(make_config(backend, profile), sample_spec,
                                        sample_spec, frame_factory, arena);
        CHECK(resampler);
        LONGS_EQUAL(status::StatusOK, resampler->init_status());

        ResamplerReader rreader(input_reader, frame_factory, *resampler, sample_spec,
                                sample_spec);
        LONGS_EQUAL(status::StatusOK, rreader.init_status());

        const FrameReadMode mode_list[] = {
            ModeHard,
            ModeSoft,
        };

        for (size_t md_n = 0; md_n < ROC_ARRAY_SIZE(mode_list); md_n++) {
            FramePtr frame = frame_factory.allocate_frame(0);
            CHECK(frame);

            LONGS_EQUAL(status::StatusOK,
                        rreader.read(*frame, OutFrameSize / NumCh, mode_list[md_n]));

            LONGS_EQUAL(mode_list[md_n], input_reader.last_mode());
        }
    }
}

// Forward error from underlying reader.
TEST(resampler, reader_forward_error) {
    enum {
        SampleRate = 44100,
        NumCh = 2,
        ChMask = 0x3,
    };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        const ResamplerBackend backend = supported_backends[n_back];
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChMask);

        test::MockReader input_reader(frame_factory, sample_spec);

        core::SharedPtr<IResampler> resampler =
            processor_map.new_resampler(make_config(backend, profile), sample_spec,
                                        sample_spec, frame_factory, arena);
        CHECK(resampler);
        LONGS_EQUAL(status::StatusOK, resampler->init_status());

        ResamplerReader rreader(input_reader, frame_factory, *resampler, sample_spec,
                                sample_spec);
        LONGS_EQUAL(status::StatusOK, rreader.init_status());

        const status::StatusCode status_list[] = {
            status::StatusDrain,
            status::StatusAbort,
        };

        for (size_t st_n = 0; st_n < ROC_ARRAY_SIZE(status_list); st_n++) {
            input_reader.set_status(status_list[st_n]);

            FramePtr frame = frame_factory.allocate_frame(0);
            CHECK(frame);

            LONGS_EQUAL(status_list[st_n],
                        rreader.read(*frame, OutFrameSize / NumCh, ModeHard));
        }
    }
}

// Forward error from underlying writer.
TEST(resampler, writer_forward_error) {
    enum {
        SampleRate = 44100,
        NumCh = 2,
        ChMask = 0x3,
    };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        const ResamplerBackend backend = supported_backends[n_back];
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChMask);

        test::MockWriter output_writer;

        core::SharedPtr<IResampler> resampler =
            processor_map.new_resampler(make_config(backend, profile), sample_spec,
                                        sample_spec, frame_factory, arena);
        CHECK(resampler);
        LONGS_EQUAL(status::StatusOK, resampler->init_status());

        ResamplerWriter rwriter(output_writer, frame_factory, *resampler, sample_spec,
                                sample_spec);
        LONGS_EQUAL(status::StatusOK, rwriter.init_status());

        output_writer.set_status(status::StatusAbort);

        for (;;) {
            FramePtr frame = new_frame(sample_spec, InFrameSize, 0, 0);
            const status::StatusCode status = rwriter.write(*frame);

            CHECK(status == status::StatusOK || status == status::StatusAbort);
            if (status == status::StatusAbort) {
                break;
            }
        }
    }
}

// If underlying reader returns partial result, resampler reader should repeat
// reading until it accumulates full frame.
TEST(resampler, reader_process_partial) {
    enum {
        SampleRate = 44100,
        NumCh = 2,
        ChMask = 0x3,
        NumIters = 50,
    };

    for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
        const ResamplerBackend backend = supported_backends[n_back];
        const ResamplerProfile profile = ResamplerProfile_High;

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChMask);

        test::MockReader input_reader(frame_factory, sample_spec);

        core::SharedPtr<IResampler> resampler =
            processor_map.new_resampler(make_config(backend, profile), sample_spec,
                                        sample_spec, frame_factory, arena);
        CHECK(resampler);
        LONGS_EQUAL(status::StatusOK, resampler->init_status());

        ResamplerReader rreader(input_reader, frame_factory, *resampler, sample_spec,
                                sample_spec);
        LONGS_EQUAL(status::StatusOK, rreader.init_status());

        input_reader.add_zero_samples();
        input_reader.set_limit(10);

        for (size_t i = 0; i < NumIters; i++) {
            FramePtr frame = read_frame(rreader, sample_spec, OutFrameSize);
            CHECK(frame);
        }
    }
}

// Attach to frame pre-allocated buffers of different sizes before reading.
TEST(resampler, reader_preallocated_buffer) {
    enum {
        SampleRate = 44100,
        NumCh = 2,
        ChMask = 0x3,
    };

    const size_t buffer_list[] = {
        OutFrameSize * 50, // big size (reader should use it)
        OutFrameSize,      // exact size (reader should use it)
        OutFrameSize - 1,  // small size (reader should replace buffer)
        0,                 // no buffer (reader should allocate buffer)
    };

    for (size_t n_buf = 0; n_buf < ROC_ARRAY_SIZE(buffer_list); n_buf++) {
        const size_t orig_buf_sz = buffer_list[n_buf];

        for (size_t n_back = 0; n_back < n_supported_backends; n_back++) {
            const ResamplerBackend backend = supported_backends[n_back];
            const ResamplerProfile profile = ResamplerProfile_High;

            const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw,
                                         ChanLayout_Surround, ChanOrder_Smpte, ChMask);

            test::MockReader input_reader(frame_factory, sample_spec);
            input_reader.add_zero_samples();

            core::SharedPtr<IResampler> resampler =
                processor_map.new_resampler(make_config(backend, profile), sample_spec,
                                            sample_spec, frame_factory, arena);
            CHECK(resampler);
            LONGS_EQUAL(status::StatusOK, resampler->init_status());

            ResamplerReader rreader(input_reader, frame_factory, *resampler, sample_spec,
                                    sample_spec);
            LONGS_EQUAL(status::StatusOK, rreader.init_status());

            FrameFactory mock_factory(arena, orig_buf_sz * sizeof(sample_t));
            FramePtr frame = orig_buf_sz > 0 ? mock_factory.allocate_frame(0)
                                             : mock_factory.allocate_frame_no_buffer();

            core::Slice<uint8_t> orig_buf = frame->buffer();

            LONGS_EQUAL(status::StatusOK,
                        rreader.read(*frame, OutFrameSize / NumCh, ModeHard));

            CHECK(frame->buffer());

            if (orig_buf_sz >= OutFrameSize) {
                CHECK(frame->buffer() == orig_buf);
            } else {
                CHECK(frame->buffer() != orig_buf);
            }

            LONGS_EQUAL(OutFrameSize / NumCh, frame->duration());
            LONGS_EQUAL(OutFrameSize, frame->num_raw_samples());
            LONGS_EQUAL(OutFrameSize * sizeof(sample_t), frame->num_bytes());
        }
    }
}

} // namespace audio
} // namespace roc
