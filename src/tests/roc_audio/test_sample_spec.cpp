/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

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

    for (size_t numChans = 1; numChans < 32; ++numChans) {
        const SampleSpec sample_spec =
            SampleSpec((size_t)SampleRate, ChanLayout_Surround, ChanOrder_Smpte,
                       ChannelMask(((uint64_t)1 << numChans) - 1));

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

    for (size_t numChans = 1; numChans < 32; ++numChans) {
        const SampleSpec sample_spec =
            SampleSpec((size_t)SampleRate, ChanLayout_Surround, ChanOrder_Smpte,
                       ChannelMask(((uint64_t)1 << numChans) - 1));

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

        // stream_timestamp_delta_2_ns
        CHECK(core::ns_equal_delta(sample_spec.stream_timestamp_delta_2_ns(1),
                                   sampling_period, epsilon));
    }
}

TEST(sample_spec, saturation) {
#if ROC_CPU_BITS == 32
    {
        const SampleSpec sample_spec = SampleSpec(
            1000000000, ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Stereo);

        // ns_2_samples_per_chan
        CHECK_EQUAL(ROC_MAX_OF(size_t),
                    sample_spec.ns_2_samples_per_chan(ROC_MAX_OF(core::nanoseconds_t)));
    }
#endif
    {
        const SampleSpec sample_spec = SampleSpec(
            1000000000, ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Stereo);

        // ns_2_samples_overall
        // (-1 because result is always multiple of channel count)
        CHECK_EQUAL(ROC_MAX_OF(size_t) - 1,
                    sample_spec.ns_2_samples_overall(ROC_MAX_OF(core::nanoseconds_t)));
    }
#if ROC_CPU_BITS == 64
    {
        const SampleSpec sample_spec =
            SampleSpec(1, ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Stereo);

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
            SampleSpec(1, ChanLayout_Surround, ChanOrder_Smpte, ChanMask_Surround_Stereo);

        // ns_2_stream_timestamp_delta
        CHECK_EQUAL(
            ROC_MAX_OF(packet::stream_timestamp_diff_t),
            sample_spec.ns_2_stream_timestamp_delta(ROC_MAX_OF(core::nanoseconds_t)));

        CHECK_EQUAL(
            ROC_MIN_OF(packet::stream_timestamp_diff_t),
            sample_spec.ns_2_stream_timestamp_delta(ROC_MIN_OF(core::nanoseconds_t)));
    }
}

} // namespace audio
} // namespace roc
