/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/sample_spec.cpp"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

TEST_GROUP(sample_spec) {};

TEST(sample_spec, ns_2_nsamples) {
    const float SampleRate = 44100;

    for (size_t numch = 1; numch < 32; ++numch) {
        const SampleSpec sample_spec = SampleSpec((size_t)SampleRate, ChanLayout_Surround,
                                                  ChannelMask((1 << numch) - 1));

        CHECK_EQUAL(sample_spec.channel_set().num_channels(), numch);

        CHECK_EQUAL(sample_spec.num_channels(), numch);

        // Check rounding
        CHECK_EQUAL(sample_spec.ns_2_samples_per_chan(
                        core::nanoseconds_t(1 / SampleRate * core::Second / 2 + 1)),
                    1);
        CHECK_EQUAL(sample_spec.ns_2_samples_per_chan(
                        core::nanoseconds_t(1 / SampleRate * core::Second / 2 - 1)),
                    0);

        CHECK_EQUAL(sample_spec.ns_2_samples_per_chan(
                        core::nanoseconds_t(1 / SampleRate * core::Second)),
                    1);
        CHECK_EQUAL(sample_spec.ns_2_samples_per_chan(
                        core::nanoseconds_t(2 / SampleRate * core::Second)),
                    2);
        CHECK_EQUAL(sample_spec.ns_2_rtp_timestamp(
                        core::nanoseconds_t(1 / SampleRate * core::Second)),
                    1);
        CHECK_EQUAL(sample_spec.ns_2_rtp_timestamp(
                        core::nanoseconds_t(2 / SampleRate * core::Second)),
                    2);
        CHECK_EQUAL(sample_spec.ns_2_samples_overall(
                        core::nanoseconds_t(1 / SampleRate * core::Second)),
                    numch);
        CHECK_EQUAL(sample_spec.ns_2_samples_overall(
                        core::nanoseconds_t(2 / SampleRate * core::Second)),
                    numch * 2);
    }
}

TEST(sample_spec, nsamples_2_ns) {
    const double SampleRate = 44100;

    core::nanoseconds_t epsilon = core::nanoseconds_t(0.01 / SampleRate * core::Second);

    for (size_t numch = 1; numch < 32; ++numch) {
        const SampleSpec sample_spec = SampleSpec((size_t)SampleRate, ChanLayout_Surround,
                                                  ChannelMask((1 << numch) - 1));

        CHECK_EQUAL(sample_spec.channel_set().num_channels(), numch);

        CHECK_EQUAL(sample_spec.num_channels(), numch);

        core::nanoseconds_t sampling_period =
            core::nanoseconds_t(1 / SampleRate * core::Second);

        CHECK(core::ns_equal_delta(sample_spec.samples_per_chan_2_ns(1), sampling_period,
                                   epsilon));
        CHECK(core::ns_equal_delta(sample_spec.fract_samples_per_chan_2_ns(0.1f),
                                   core::nanoseconds_t(0.1 / SampleRate * core::Second),
                                   epsilon));
        CHECK(core::ns_equal_delta(sample_spec.fract_samples_per_chan_2_ns(-0.1f),
                                   -core::nanoseconds_t(0.1 / SampleRate * core::Second),
                                   epsilon));
        CHECK(core::ns_equal_delta(sample_spec.samples_overall_2_ns(numch),
                                   sampling_period, epsilon));
        CHECK(core::ns_equal_delta(sample_spec.rtp_timestamp_2_ns(1), sampling_period,
                                   epsilon));
    }
}

} // namespace audio
} // namespace roc
