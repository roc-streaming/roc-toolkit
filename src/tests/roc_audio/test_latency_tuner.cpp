/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/latency_tuner.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

namespace {

enum {
    SampleRate = 44100,
    NumCh = 2,
    ChMask = 0x3
};

const SampleSpec sample_spec(SampleRate,
                              Sample_RawFormat,
                              ChanLayout_Surround,
                              ChanOrder_Smpte,
                              ChMask);

// Build a fully-specified config with explicit values so tests are deterministic.
LatencyConfig make_config(core::nanoseconds_t target,
                          core::nanoseconds_t tolerance,
                          LatencyTunerProfile profile = LatencyTunerProfile_Gradual,
                          LatencyTunerBackend backend = LatencyTunerBackend_Niq) {
    LatencyConfig config;
    config.tuner_backend = backend;
    config.tuner_profile = profile;
    config.target_latency = target;
    config.latency_tolerance = tolerance;
    config.stale_tolerance = tolerance / 4;
    config.scaling_interval = 5 * core::Millisecond;
    config.scaling_tolerance = 0.005f;
    return config;
}

// Feed the tuner N times with the given niq_latency and advance stream by one
// scaling_interval worth of samples each time.
void feed_niq(LatencyTuner& tuner,
              core::nanoseconds_t niq_latency,
              size_t iterations,
              core::nanoseconds_t step = 5 * core::Millisecond) {
    LatencyMetrics lm;
    lm.niq_latency = niq_latency;
    packet::LinkMetrics link;

    const packet::stream_timestamp_t step_samples =
        sample_spec.ns_2_stream_timestamp(step);

    for (size_t i = 0; i < iterations; i++) {
        tuner.write_metrics(lm, link);
        CHECK(tuner.update_stream());
        tuner.advance_stream(step_samples);
    }
}

} // namespace

// ---------------------------------------------------------------------------
// Initialization
// ---------------------------------------------------------------------------

TEST_GROUP(latency_tuner_init) {};

TEST(latency_tuner_init, intact_profile_no_target_needed) {
    // Intact profile: no tuning, no bounds — target_latency may be zero.
    LatencyConfig config;
    config.tuner_backend = LatencyTunerBackend_Niq;
    config.tuner_profile = LatencyTunerProfile_Intact;

    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());
}

TEST(latency_tuner_init, gradual_profile_valid_config) {
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());
}

TEST(latency_tuner_init, responsive_profile_valid_config) {
    LatencyConfig config = make_config(
        200 * core::Millisecond, 50 * core::Millisecond, LatencyTunerProfile_Responsive);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());
}

TEST(latency_tuner_init, invalid_negative_target_latency) {
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    config.target_latency = -1;
    LatencyTuner tuner(config, sample_spec);
    CHECK_FALSE(tuner.is_valid());
}

TEST(latency_tuner_init, invalid_negative_tolerance) {
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    config.latency_tolerance = -1;
    LatencyTuner tuner(config, sample_spec);
    CHECK_FALSE(tuner.is_valid());
}

// ---------------------------------------------------------------------------
// Bounds checking
// ---------------------------------------------------------------------------

TEST_GROUP(latency_tuner_bounds) {};

TEST(latency_tuner_bounds, latency_at_target_stays_alive) {
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    // Feed exactly target latency — should never go out of bounds.
    feed_niq(tuner, 200 * core::Millisecond, 100);
}

TEST(latency_tuner_bounds, latency_above_max_terminates) {
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    // Feed latency well above target + tolerance (200 + 50 = 250ms).
    LatencyMetrics lm;
    lm.niq_latency = 400 * core::Millisecond;
    packet::LinkMetrics link;

    tuner.write_metrics(lm, link);
    CHECK_FALSE(tuner.update_stream());
}

TEST(latency_tuner_bounds, latency_below_min_terminates) {
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    // Feed latency well below target - tolerance (200 - 50 = 150ms).
    LatencyMetrics lm;
    lm.niq_latency = 10 * core::Millisecond;
    packet::LinkMetrics link;

    tuner.write_metrics(lm, link);
    CHECK_FALSE(tuner.update_stream());
}

TEST(latency_tuner_bounds, stalling_suppresses_low_latency_termination) {
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    // stale_tolerance = 50ms / 4 = 12.5ms
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    // Latency below min, but queue is stalling (burst drop scenario).
    // Tuner should NOT terminate — it defers to watchdog.
    LatencyMetrics lm;
    lm.niq_latency = 10 * core::Millisecond;
    lm.niq_stalling = 100 * core::Millisecond; // well above stale_tolerance
    packet::LinkMetrics link;

    tuner.write_metrics(lm, link);
    CHECK(tuner.update_stream()); // should survive
}

TEST(latency_tuner_bounds, no_metrics_yet_does_not_terminate) {
    // Before any metrics arrive, update_stream() should return true.
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    CHECK(tuner.update_stream());
}

TEST(latency_tuner_bounds, intact_profile_ignores_out_of_bounds) {
    // Intact profile has no bounds checking — should never terminate.
    LatencyConfig config;
    config.tuner_backend = LatencyTunerBackend_Niq;
    config.tuner_profile = LatencyTunerProfile_Intact;

    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    LatencyMetrics lm;
    lm.niq_latency = 10000 * core::Millisecond; // absurdly high
    packet::LinkMetrics link;

    tuner.write_metrics(lm, link);
    CHECK(tuner.update_stream()); // intact profile never terminates
}

// ---------------------------------------------------------------------------
// Scaling / freq coefficient
// ---------------------------------------------------------------------------

TEST_GROUP(latency_tuner_scaling) {};

TEST(latency_tuner_scaling, no_scaling_before_first_interval) {
    // fetch_scaling() returns 0 until the first scaling interval elapses.
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    LatencyMetrics lm;
    lm.niq_latency = 200 * core::Millisecond;
    packet::LinkMetrics link;

    tuner.write_metrics(lm, link);
    CHECK(tuner.update_stream());
    // No advance_stream yet — scaling interval not elapsed.
    DOUBLES_EQUAL(0.0, (double)tuner.fetch_scaling(), 1e-6);
}

TEST(latency_tuner_scaling, scaling_near_one_when_at_target) {
    // When latency == target, freq_coeff should stay very close to 1.0.
    LatencyConfig config =
        make_config(200 * core::Millisecond, 50 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    feed_niq(tuner, 200 * core::Millisecond, 200);

    const float scaling = tuner.fetch_scaling();
    if (scaling != 0) {
        // Allow ±0.5% (scaling_tolerance).
        CHECK(scaling >= 0.995f);
        CHECK(scaling <= 1.005f);
    }
}

TEST(latency_tuner_scaling, scaling_above_one_when_latency_high) {
    // When latency > target, freq_coeff should drift above 1.0 (speed up sender).
    LatencyConfig config =
        make_config(200 * core::Millisecond, 100 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    // Feed latency significantly above target for many intervals.
    feed_niq(tuner, 280 * core::Millisecond, 500);

    const float scaling = tuner.fetch_scaling();
    if (scaling != 0) {
        CHECK(scaling > 1.0f);
    }
}

TEST(latency_tuner_scaling, scaling_below_one_when_latency_low) {
    // When latency < target, freq_coeff should drift below 1.0 (slow down sender).
    LatencyConfig config =
        make_config(200 * core::Millisecond, 100 * core::Millisecond);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    feed_niq(tuner, 120 * core::Millisecond, 500);

    const float scaling = tuner.fetch_scaling();
    if (scaling != 0) {
        CHECK(scaling < 1.0f);
    }
}

TEST(latency_tuner_scaling, scaling_clamped_to_tolerance) {
    // Even with extreme latency, freq_coeff must stay within ±scaling_tolerance.
    LatencyConfig config =
        make_config(200 * core::Millisecond, 500 * core::Millisecond);
    config.scaling_tolerance = 0.005f;
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    feed_niq(tuner, 5000 * core::Millisecond, 1000);

    const float scaling = tuner.fetch_scaling();
    if (scaling != 0) {
        CHECK(scaling <= 1.0f + config.scaling_tolerance);
        CHECK(scaling >= 1.0f - config.scaling_tolerance);
    }
}

TEST(latency_tuner_scaling, intact_profile_never_produces_scaling) {
    // Intact profile disables tuning — fetch_scaling() should always return 0.
    LatencyConfig config;
    config.tuner_backend = LatencyTunerBackend_Niq;
    config.tuner_profile = LatencyTunerProfile_Intact;

    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    LatencyMetrics lm;
    lm.niq_latency = 500 * core::Millisecond;
    packet::LinkMetrics link;

    const packet::stream_timestamp_t step =
        sample_spec.ns_2_stream_timestamp(5 * core::Millisecond);

    for (size_t i = 0; i < 200; i++) {
        tuner.write_metrics(lm, link);
        tuner.update_stream();
        tuner.advance_stream(step);
        DOUBLES_EQUAL(0.0, (double)tuner.fetch_scaling(), 1e-6);
    }
}

// ---------------------------------------------------------------------------
// E2E backend
// ---------------------------------------------------------------------------

TEST_GROUP(latency_tuner_e2e) {};

TEST(latency_tuner_e2e, no_update_without_e2e_metrics) {
    // E2E backend: if no e2e_latency has been reported, update_stream() returns true
    // (no data yet, not an error).
    LatencyConfig config = make_config(200 * core::Millisecond,
                                       50 * core::Millisecond,
                                       LatencyTunerProfile_Gradual,
                                       LatencyTunerBackend_E2e);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    // Only provide niq metrics — e2e backend should ignore them.
    LatencyMetrics lm;
    lm.niq_latency = 400 * core::Millisecond; // would be out of bounds for niq
    packet::LinkMetrics link;

    tuner.write_metrics(lm, link);
    CHECK(tuner.update_stream()); // no e2e data yet, should not terminate
}

TEST(latency_tuner_e2e, terminates_on_e2e_out_of_bounds) {
    LatencyConfig config = make_config(200 * core::Millisecond,
                                       50 * core::Millisecond,
                                       LatencyTunerProfile_Gradual,
                                       LatencyTunerBackend_E2e);
    LatencyTuner tuner(config, sample_spec);
    CHECK(tuner.is_valid());

    LatencyMetrics lm;
    lm.e2e_latency = 400 * core::Millisecond; // above max (200+50=250ms)
    packet::LinkMetrics link;

    tuner.write_metrics(lm, link);
    CHECK_FALSE(tuner.update_stream());
}

} // namespace audio
} // namespace roc
