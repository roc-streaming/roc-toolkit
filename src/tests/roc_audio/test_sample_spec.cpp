/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>
#include <CppUTest/UtestMacros.h>

#include "roc_audio/channel_defs.h"
#include "roc_audio/format.h"
#include "roc_audio/pcm_subformat.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.cpp"
#include "roc_core/cpu_traits.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

TEST_GROUP(sample_spec) {};

TEST(sample_spec, ns_2_nsamples) {
    const double SampleRate = 44100;

    for (size_t numChans = 1; numChans < ChanPos_Max; ++numChans) {
        const SampleSpec sample_spec =
            SampleSpec((size_t)SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                       ChanOrder_Smpte, ChannelMask(((uint64_t)1 << numChans) - 1));

        // num_channels
        CHECK_EQUAL(numChans, sample_spec.channel_set().num_channels());
        CHECK_EQUAL(numChans, sample_spec.num_channels());

        // rounding
        CHECK_EQUAL(1,
                    sample_spec.ns_2_samples_per_chan(
                        core::nanoseconds_t(1 / SampleRate * core::Second / 2 + 1)));
        CHECK_EQUAL(0,
                    sample_spec.ns_2_samples_per_chan(
                        core::nanoseconds_t(1 / SampleRate * core::Second / 2 - 1)));

        // ns_2_samples_per_chan
        CHECK_EQUAL(1,
                    sample_spec.ns_2_samples_per_chan(
                        core::nanoseconds_t(1 / SampleRate * core::Second)));
        CHECK_EQUAL(2,
                    sample_spec.ns_2_samples_per_chan(
                        core::nanoseconds_t(2 / SampleRate * core::Second)));

        // ns_2_samples_overall
        CHECK_EQUAL(numChans,
                    sample_spec.ns_2_samples_overall(
                        core::nanoseconds_t(1 / SampleRate * core::Second)));
        CHECK_EQUAL(numChans * 2,
                    sample_spec.ns_2_samples_overall(
                        core::nanoseconds_t(2 / SampleRate * core::Second)));

        // ns_2_stream_timestamp
        CHECK_EQUAL(1,
                    sample_spec.ns_2_stream_timestamp(
                        core::nanoseconds_t(1 / SampleRate * core::Second)));
        CHECK_EQUAL(2,
                    sample_spec.ns_2_stream_timestamp(
                        core::nanoseconds_t(2 / SampleRate * core::Second)));

        // ns_2_stream_timestamp_delta
        CHECK_EQUAL(1,
                    sample_spec.ns_2_stream_timestamp_delta(
                        core::nanoseconds_t(1 / SampleRate * core::Second)));
        CHECK_EQUAL(2,
                    sample_spec.ns_2_stream_timestamp_delta(
                        core::nanoseconds_t(2 / SampleRate * core::Second)));
    }
}

TEST(sample_spec, nsamples_2_ns) {
    const double SampleRate = 44100;

    core::nanoseconds_t epsilon = core::nanoseconds_t(0.01 / SampleRate * core::Second);

    for (size_t numChans = 1; numChans < ChanPos_Max; ++numChans) {
        const SampleSpec sample_spec =
            SampleSpec((size_t)SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                       ChanOrder_Smpte, ChannelMask(((uint64_t)1 << numChans) - 1));

        const core::nanoseconds_t sampling_period =
            core::nanoseconds_t(1 / SampleRate * core::Second);

        // num_channels
        CHECK_EQUAL(numChans, sample_spec.channel_set().num_channels());
        CHECK_EQUAL(numChans, sample_spec.num_channels());

        // samples_per_chan_2_ns
        CHECK(core::ns_equal_delta(sample_spec.samples_per_chan_2_ns(1), sampling_period,
                                   epsilon));

        // fract_samples_per_chan_2_ns
        CHECK(core::ns_equal_delta(sample_spec.fract_samples_per_chan_2_ns(0.1f),
                                   core::nanoseconds_t(0.1 / SampleRate * core::Second),
                                   epsilon));
        CHECK(core::ns_equal_delta(sample_spec.fract_samples_per_chan_2_ns(-0.1f),
                                   -core::nanoseconds_t(0.1 / SampleRate * core::Second),
                                   epsilon));

        // samples_overall_2_ns
        CHECK(core::ns_equal_delta(sample_spec.samples_overall_2_ns(numChans),
                                   sampling_period, epsilon));

        // fract_samples_overall_2_ns
        CHECK(core::ns_equal_delta(
            sample_spec.fract_samples_overall_2_ns(0.1f),
            core::nanoseconds_t(0.1 / SampleRate * core::Second / numChans), epsilon));
        CHECK(core::ns_equal_delta(
            sample_spec.fract_samples_overall_2_ns(-0.1f),
            -core::nanoseconds_t(0.1 / SampleRate * core::Second / numChans), epsilon));

        // stream_timestamp_2_ns
        CHECK(core::ns_equal_delta(sample_spec.stream_timestamp_2_ns(1), sampling_period,
                                   epsilon));

        // stream_timestamp_delta_2_ns
        CHECK(core::ns_equal_delta(sample_spec.stream_timestamp_delta_2_ns(1),
                                   sampling_period, epsilon));
    }
}

TEST(sample_spec, saturation) {
#if ROC_CPU_BITS == 32
    {
        const SampleSpec sample_spec =
            SampleSpec(1000000000, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                       ChanMask_Surround_Stereo);

        // ns_2_samples_per_chan
        CHECK_EQUAL(ROC_MAX_OF(size_t),
                    sample_spec.ns_2_samples_per_chan(ROC_MAX_OF(core::nanoseconds_t)));
    }
#endif
    {
        const SampleSpec sample_spec =
            SampleSpec(1000000000, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                       ChanMask_Surround_Stereo);

        // ns_2_samples_overall
        // (-1 because result is always multiple of channel count)
        CHECK_EQUAL(ROC_MAX_OF(size_t) - 1,
                    sample_spec.ns_2_samples_overall(ROC_MAX_OF(core::nanoseconds_t)));
    }
#if ROC_CPU_BITS == 64
    {
        const SampleSpec sample_spec =
            SampleSpec(1, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                       ChanMask_Surround_Stereo);

        // samples_per_chan_2_ns
        CHECK_EQUAL(ROC_MAX_OF(core::nanoseconds_t),
                    sample_spec.samples_per_chan_2_ns(ROC_MAX_OF(size_t)));

        // fract_samples_per_chan_2_ns
        CHECK_EQUAL(ROC_MAX_OF(core::nanoseconds_t),
                    sample_spec.fract_samples_per_chan_2_ns((float)ROC_MAX_OF(size_t)));

        // samples_overall_2_ns
        CHECK_EQUAL(ROC_MAX_OF(core::nanoseconds_t),
                    sample_spec.samples_overall_2_ns(ROC_MAX_OF(size_t) - 1));

        // fract_samples_overall_2_ns
        CHECK_EQUAL(
            ROC_MAX_OF(core::nanoseconds_t),
            sample_spec.fract_samples_overall_2_ns((float)ROC_MAX_OF(size_t) - 1));
    }
#endif
    {
        const SampleSpec sample_spec =
            SampleSpec(1, PcmSubformat_Raw, ChanLayout_Surround, ChanOrder_Smpte,
                       ChanMask_Surround_Stereo);

        // ns_2_stream_timestamp_delta
        CHECK_EQUAL(
            ROC_MAX_OF(packet::stream_timestamp_diff_t),
            sample_spec.ns_2_stream_timestamp_delta(ROC_MAX_OF(core::nanoseconds_t)));

        CHECK_EQUAL(
            ROC_MIN_OF(packet::stream_timestamp_diff_t),
            sample_spec.ns_2_stream_timestamp_delta(ROC_MIN_OF(core::nanoseconds_t)));
    }
}

TEST(sample_spec, bytes) {
    { // raw format
        const size_t SampleRate = 44100;
        const size_t NumChans = 2;
        const size_t SampleSize = sizeof(sample_t);

        const SampleSpec sample_spec(SampleRate, PcmSubformat_Raw, ChanLayout_Surround,
                                     ChanOrder_Smpte, ChanMask_Surround_Stereo);

        // bytes_2_stream_timestamp
        CHECK_EQUAL(111,
                    sample_spec.bytes_2_stream_timestamp(111 * NumChans * SampleSize));

        // stream_timestamp_2_bytes
        CHECK_EQUAL(111 * NumChans * SampleSize,
                    sample_spec.stream_timestamp_2_bytes(111));

        // bytes_2_ns
        CHECK_EQUAL(sample_spec.stream_timestamp_2_ns(111),
                    sample_spec.bytes_2_ns(111 * NumChans * SampleSize));

        // ns_2_bytes
        CHECK_EQUAL(111 * NumChans * SampleSize,
                    sample_spec.ns_2_bytes(sample_spec.stream_timestamp_2_ns(111)));
    }
    { // alternative format
        const size_t SampleRate = 44100;
        const size_t NumChans = 2;
        const size_t SampleSize = 3; // 24 bits

        const SampleSpec sample_spec(SampleRate, PcmSubformat_SInt24_Be,
                                     ChanLayout_Surround, ChanOrder_Smpte,
                                     ChanMask_Surround_Stereo);

        // bytes_2_stream_timestamp
        CHECK_EQUAL(111,
                    sample_spec.bytes_2_stream_timestamp(111 * NumChans * SampleSize));

        // stream_timestamp_2_bytes
        CHECK_EQUAL(111 * NumChans * SampleSize,
                    sample_spec.stream_timestamp_2_bytes(111));

        // bytes_2_ns
        CHECK_EQUAL(sample_spec.stream_timestamp_2_ns(111),
                    sample_spec.bytes_2_ns(111 * NumChans * SampleSize));

        // ns_2_bytes
        CHECK_EQUAL(111 * NumChans * SampleSize,
                    sample_spec.ns_2_bytes(sample_spec.stream_timestamp_2_ns(111)));
    }
}

TEST(sample_spec, empty_complete) {
    SampleSpec sample_spec;

    // sample spec is empty
    CHECK(!sample_spec.is_complete());
    CHECK(sample_spec.is_empty());
    CHECK_EQUAL(0, sample_spec.sample_rate());
    CHECK_EQUAL(Format_Invalid, sample_spec.format());
    CHECK_EQUAL(PcmSubformat_Invalid, sample_spec.pcm_subformat());
    CHECK_EQUAL(ChanLayout_None, sample_spec.channel_set().layout());
    CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());
    CHECK_EQUAL(0, sample_spec.channel_set().num_channels());

    // set all fields
    sample_spec.set_sample_rate(44100);
    CHECK(!sample_spec.is_complete());
    CHECK(!sample_spec.is_empty());

    sample_spec.set_format(Format_Pcm);
    CHECK(!sample_spec.is_complete());
    CHECK(!sample_spec.is_empty());

    sample_spec.set_pcm_subformat(PcmSubformat_Float32);
    CHECK(!sample_spec.is_complete());
    CHECK(!sample_spec.is_empty());

    sample_spec.channel_set().set_layout(ChanLayout_Surround);
    CHECK(!sample_spec.is_complete());
    CHECK(!sample_spec.is_empty());

    sample_spec.channel_set().set_order(ChanOrder_Smpte);
    CHECK(!sample_spec.is_complete());
    CHECK(!sample_spec.is_empty());

    sample_spec.channel_set().set_mask(ChanMask_Surround_Stereo);
    CHECK(sample_spec.is_complete());
    CHECK(!sample_spec.is_empty());

    // sample spec is complete
    CHECK(sample_spec.is_complete());
    CHECK(!sample_spec.is_empty());
    CHECK_EQUAL(44100, sample_spec.sample_rate());
    CHECK_EQUAL(Format_Pcm, sample_spec.format());
    CHECK_EQUAL(PcmSubformat_Float32, sample_spec.pcm_subformat());
    CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
    CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
    CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));

    // clear all fields
    sample_spec.clear();

    // sample spec is empty
    CHECK(!sample_spec.is_complete());
    CHECK(sample_spec.is_empty());
    CHECK_EQUAL(0, sample_spec.sample_rate());
    CHECK_EQUAL(Format_Invalid, sample_spec.format());
    CHECK_EQUAL(PcmSubformat_Invalid, sample_spec.pcm_subformat());
    CHECK_EQUAL(ChanLayout_None, sample_spec.channel_set().layout());
    CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());
    CHECK_EQUAL(0, sample_spec.channel_set().num_channels());
}

TEST(sample_spec, pcm_raw) {
    { // empty
        SampleSpec sample_spec;
        CHECK(!sample_spec.is_complete());
        CHECK(sample_spec.is_empty());
        CHECK(!sample_spec.is_pcm());
        CHECK(!sample_spec.is_raw());
    }
    { // incomplete (pcm, raw)
        SampleSpec sample_spec;
        sample_spec.set_format(Format_Pcm);
        sample_spec.set_pcm_subformat(PcmSubformat_Float32);
        CHECK(!sample_spec.is_complete());
        CHECK(!sample_spec.is_empty());
        CHECK(sample_spec.is_pcm());
        CHECK(sample_spec.is_raw());
    }
    { // complete (pcm, raw)
        SampleSpec sample_spec;
        sample_spec.set_format(Format_Pcm);
        sample_spec.set_pcm_subformat(PcmSubformat_Float32);
        sample_spec.set_sample_rate(44100);
        sample_spec.set_channel_set(
            ChannelSet(ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Stereo));
        CHECK(sample_spec.is_complete());
        CHECK(!sample_spec.is_empty());
        CHECK(sample_spec.is_pcm());
        CHECK(sample_spec.is_raw());
    }
    { // format mismatch (non-pcm)
        SampleSpec sample_spec;
        sample_spec.set_format(Format_Wav);
        sample_spec.set_pcm_subformat(PcmSubformat_Float32);
        sample_spec.set_sample_rate(44100);
        sample_spec.set_channel_set(
            ChannelSet(ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Stereo));
        CHECK(sample_spec.is_complete());
        CHECK(!sample_spec.is_empty());
        CHECK(!sample_spec.is_pcm());
        CHECK(!sample_spec.is_raw());
    }
    { // sub-format mismatch (pcm, non-raw)
        SampleSpec sample_spec;
        sample_spec.set_format(Format_Pcm);
        sample_spec.set_pcm_subformat(PcmSubformat_Float32_Be);
        sample_spec.set_sample_rate(44100);
        sample_spec.set_channel_set(
            ChannelSet(ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Stereo));
        CHECK(sample_spec.is_complete());
        CHECK(!sample_spec.is_empty());
        CHECK(sample_spec.is_pcm());
        CHECK(!sample_spec.is_raw());
    }
}

TEST(sample_spec, parse_rate) {
    { // 44.1Khz
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/44100/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(44100, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));
    }
    { // 48Khz
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));
    }
}

TEST(sample_spec, parse_format) {
    { // format: pcm, subformat: uint8
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@u8/44100/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(44100, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_UInt8, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));

        STRCMP_EQUAL("pcm", sample_spec.format_name());
        STRCMP_EQUAL("u8", sample_spec.subformat_name());
    }
    { // format: pcm, subformat: sint18_4_le
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s18_4le/48000/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt18_4_Le, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));

        STRCMP_EQUAL("pcm", sample_spec.format_name());
        STRCMP_EQUAL("s18_4le", sample_spec.subformat_name());
    }
    { // format: wav, subformat: float32
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("wav@f32/48000/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Wav, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_Float32, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));

        STRCMP_EQUAL("wav", sample_spec.format_name());
        STRCMP_EQUAL("f32", sample_spec.subformat_name());
    }
    { // format: custom, subformat: float64
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("test@f64/48000/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Custom, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_Float64, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));

        STRCMP_EQUAL("test", sample_spec.format_name());
        STRCMP_EQUAL("f64", sample_spec.subformat_name());
    }
    { // format: custom, subformat: custom
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("xxx@yyy/48000/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Custom, sample_spec.format());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));

        STRCMP_EQUAL("xxx", sample_spec.format_name());
        STRCMP_EQUAL("yyy", sample_spec.subformat_name());
    }
}

IGNORE_TEST(sample_spec, parse_channels) {
    { // surround stereo
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));
    }
    { // surround 5.1.2
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/surround5.1.2", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_5_1_2));
    }
    { // surround channel list
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/FL,FC,FR", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());

        CHECK_EQUAL(3, sample_spec.num_channels());
        CHECK(sample_spec.channel_set().has_channel(ChanPos_FrontLeft));
        CHECK(sample_spec.channel_set().has_channel(ChanPos_FrontCenter));
        CHECK(sample_spec.channel_set().has_channel(ChanPos_FrontRight));
    }
    { // multitrack channel list
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/1,2,3", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Multitrack, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());

        CHECK_EQUAL(3, sample_spec.num_channels());
        CHECK(sample_spec.channel_set().has_channel(1));
        CHECK(sample_spec.channel_set().has_channel(2));
        CHECK(sample_spec.channel_set().has_channel(3));
    }
    { // multitrack channel range
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/1-3", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Multitrack, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());

        CHECK_EQUAL(3, sample_spec.num_channels());
        CHECK(sample_spec.channel_set().has_channel(1));
        CHECK(sample_spec.channel_set().has_channel(2));
        CHECK(sample_spec.channel_set().has_channel(3));
    }
    { // multitrack channel list and range
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/1,3-5,7", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Multitrack, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());

        CHECK_EQUAL(5, sample_spec.num_channels());
        CHECK(sample_spec.channel_set().has_channel(1));
        CHECK(sample_spec.channel_set().has_channel(3));
        CHECK(sample_spec.channel_set().has_channel(4));
        CHECK(sample_spec.channel_set().has_channel(5));
        CHECK(sample_spec.channel_set().has_channel(7));
    }
    { // multitrack mask (zero)
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/0x00", sample_spec));

        CHECK(!sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Multitrack, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());
        CHECK_EQUAL(0, sample_spec.num_channels());
    }
    { // multitrack mask (short)
        SampleSpec sample_spec;
        // 0xAC = 10101100
        CHECK(parse_sample_spec("pcm@s16/48000/0xAC", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Multitrack, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());

        CHECK_EQUAL(4, sample_spec.num_channels());
        CHECK(sample_spec.channel_set().has_channel(2));
        CHECK(sample_spec.channel_set().has_channel(3));
        CHECK(sample_spec.channel_set().has_channel(5));
        CHECK(sample_spec.channel_set().has_channel(7));
    }
    { // multitrack mask (long)
        SampleSpec sample_spec;
        // 1010, 80 zero bits, 1100
        CHECK(parse_sample_spec("pcm@s16/48000/0xA00000000000000000000C", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Multitrack, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());

        CHECK_EQUAL(4, sample_spec.num_channels());
        CHECK(sample_spec.channel_set().has_channel(2));
        CHECK(sample_spec.channel_set().has_channel(3));
        CHECK(sample_spec.channel_set().has_channel(85));
        CHECK(sample_spec.channel_set().has_channel(87));
    }
}

TEST(sample_spec, parse_defaults) {
    { // no format
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("-/44100/stereo", sample_spec));

        CHECK(!sample_spec.is_complete());

        CHECK_EQUAL(44100, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Invalid, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_Invalid, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));
    }
    { // no subformat (wav format)
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("wav/44100/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(44100, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Wav, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_Invalid, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));
    }
    { // no subformat (custom format)
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("test/44100/stereo", sample_spec));

        CHECK(sample_spec.is_complete());

        CHECK_EQUAL(44100, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Custom, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_Invalid, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));
    }
    { // no rate
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/-/stereo", sample_spec));

        CHECK(!sample_spec.is_complete());

        CHECK_EQUAL(0, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_Surround, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_Smpte, sample_spec.channel_set().order());
        CHECK(sample_spec.channel_set().is_equal(ChanMask_Surround_Stereo));
    }
    { // no channels
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/48000/-", sample_spec));

        CHECK(!sample_spec.is_complete());

        CHECK_EQUAL(48000, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Pcm, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_SInt16, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_None, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());
        CHECK_EQUAL(0, sample_spec.channel_set().num_channels());
    }
    { // no nothing
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("-/-/-", sample_spec));

        CHECK(!sample_spec.is_complete());

        CHECK_EQUAL(0, sample_spec.sample_rate());
        CHECK_EQUAL(Format_Invalid, sample_spec.format());
        CHECK_EQUAL(PcmSubformat_Invalid, sample_spec.pcm_subformat());
        CHECK_EQUAL(ChanLayout_None, sample_spec.channel_set().layout());
        CHECK_EQUAL(ChanOrder_None, sample_spec.channel_set().order());
        CHECK_EQUAL(0, sample_spec.channel_set().num_channels());
    }
}

TEST(sample_spec, has_field) {
    { // all fields
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/44100/stereo", sample_spec));

        CHECK(sample_spec.has_format());
        CHECK(sample_spec.has_subformat());
        CHECK(sample_spec.has_sample_rate());
        CHECK(sample_spec.has_channel_set());
    }
    { // no sub-format
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("wav/44100/stereo", sample_spec));

        CHECK(sample_spec.has_format());
        CHECK(!sample_spec.has_subformat());
        CHECK(sample_spec.has_sample_rate());
        CHECK(sample_spec.has_channel_set());
    }
    { // no rate
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/-/stereo", sample_spec));

        CHECK(sample_spec.has_format());
        CHECK(sample_spec.has_subformat());
        CHECK(!sample_spec.has_sample_rate());
        CHECK(sample_spec.has_channel_set());
    }
    { // no channels
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("pcm@s16/44100/-", sample_spec));

        CHECK(sample_spec.has_format());
        CHECK(sample_spec.has_subformat());
        CHECK(sample_spec.has_sample_rate());
        CHECK(!sample_spec.has_channel_set());
    }
    { // no fields
        SampleSpec sample_spec;
        CHECK(parse_sample_spec("-/-/-", sample_spec));

        CHECK(!sample_spec.has_format());
        CHECK(!sample_spec.has_subformat());
        CHECK(!sample_spec.has_sample_rate());
        CHECK(!sample_spec.has_channel_set());
    }
}

TEST(sample_spec, parse_errors) {
    SampleSpec sample_spec;

    { // bad syntax
        CHECK(!parse_sample_spec("", sample_spec));
        CHECK(!parse_sample_spec("/", sample_spec));
        CHECK(!parse_sample_spec("//", sample_spec));
        CHECK(!parse_sample_spec("///", sample_spec));
        CHECK(!parse_sample_spec("/48000/stereo", sample_spec));
        CHECK(!parse_sample_spec("pcm@/48000/stereo", sample_spec));
        CHECK(!parse_sample_spec("@s16/48000/stereo", sample_spec));
        CHECK(!parse_sample_spec("@/48000/stereo", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16//stereo", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/48000/", sample_spec));
        CHECK(!parse_sample_spec("/pcm@s16/48000/stereo", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/48000/stereo/", sample_spec));
        CHECK(!parse_sample_spec("pcm@/48000/stereo", sample_spec));
        CHECK(!parse_sample_spec("@s16/48000/stereo", sample_spec));
    }
    { // bad rate
        CHECK(!parse_sample_spec("pcm@s16/0/stereo", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/-1/stereo", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/bad/stereo", sample_spec));
    }
    { // bad format
        CHECK(!parse_sample_spec("!!!@s16/44100/stereo", sample_spec));
        CHECK(!parse_sample_spec("!!!/44100/stereo", sample_spec));
    }
    { // bad subformat
        CHECK(!parse_sample_spec("pcm@s77/44100/stereo", sample_spec));
        CHECK(!parse_sample_spec("pcm@xxx/44100/stereo", sample_spec));
        CHECK(!parse_sample_spec("pcm/44100/stereo", sample_spec));
    }
    { // bad surround
        CHECK(!parse_sample_spec("pcm@s16/44100/bad", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/BAD,BAD", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/stereo,", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/FL,FR,", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/,FL,FR", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/FL,,FR", sample_spec));
    }
    { // bad multitrack
        CHECK(!parse_sample_spec("pcm@s16/44100/1,2,", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/,1,2", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/1,,2", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/1-", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/-2", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/1--2", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/10000", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/10000-20000", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/0x", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/0XF", sample_spec));
        CHECK(!parse_sample_spec("pcm@s16/44100/0xZZ", sample_spec));
    }
}

} // namespace audio
} // namespace roc
