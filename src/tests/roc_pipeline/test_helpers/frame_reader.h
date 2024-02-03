/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_FRAME_READER_H_
#define ROC_PIPELINE_TEST_HELPERS_FRAME_READER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_core/time.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {
namespace test {

// Read audio frames from source and validate.
class FrameReader : public core::NonCopyable<> {
public:
    FrameReader(sndio::ISource& source,
                core::BufferFactory<audio::sample_t>& buffer_factory)
        : source_(source)
        , buffer_factory_(buffer_factory)
        , offset_(0)
        , abs_offset_(0)
        // By default, we set base_cts_ to some non-zero value, so that if base_capture_ts
        // is never provided to methods, refresh_ts() will still produce valid non-zero
        // CTS even in tests that don't bother about timestamps. However, if a test
        // provides specific value for base_capture_ts, default value is overwritten.
        , base_cts_(core::Second)
        , refresh_ts_offset_(0)
        , last_capture_ts_(0) {
    }

    // Read num_samples samples.
    // Expect specific value of each sample (nth_sample() * num_session).
    // If base_capture_ts is -1, expect zero CTS, otherwise expect
    // CTS = base_capture_ts + sample offset.
    void read_samples(size_t num_samples,
                      size_t num_sessions,
                      const audio::SampleSpec& sample_spec,
                      core::nanoseconds_t base_capture_ts = -1) {
        core::Slice<audio::sample_t> samples = alloc_samples_(num_samples, sample_spec);
        audio::Frame frame(samples.data(), samples.size());
        CHECK(source_.read(frame));

        check_duration_(frame, sample_spec);
        check_timestamp_(frame, sample_spec, base_capture_ts);

        for (size_t ns = 0; ns < num_samples; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                DOUBLES_EQUAL(
                    (double)nth_sample(offset_) * num_sessions,
                    (double)frame.raw_samples()[ns * sample_spec.num_channels() + nc],
                    SampleEpsilon);
            }
            offset_++;
        }
        abs_offset_ += num_samples;
        refresh_ts_offset_ = sample_spec.samples_per_chan_2_ns(abs_offset_);

        if (base_capture_ts > 0) {
            base_cts_ = base_capture_ts;
        }
    }

    // Read num_samples samples.
    // Expect any non-zero values.
    // If base_capture_ts is -1, expect zero CTS, otherwise expect
    // CTS = base_capture_ts + sample offset.
    void read_nonzero_samples(size_t num_samples,
                              const audio::SampleSpec& sample_spec,
                              core::nanoseconds_t base_capture_ts = -1) {
        core::Slice<audio::sample_t> samples = alloc_samples_(num_samples, sample_spec);
        audio::Frame frame(samples.data(), samples.size());
        CHECK(source_.read(frame));

        check_duration_(frame, sample_spec);
        check_timestamp_(frame, sample_spec, base_capture_ts);

        size_t non_zero = 0;
        for (size_t ns = 0; ns < num_samples * sample_spec.num_channels(); ns++) {
            if (frame.raw_samples()[ns] != 0) {
                non_zero++;
            }
        }
        CHECK(non_zero > 0);
        abs_offset_ += num_samples;
        refresh_ts_offset_ = sample_spec.samples_per_chan_2_ns(abs_offset_);

        if (base_capture_ts > 0) {
            base_cts_ = base_capture_ts;
        }
    }

    // Read num_samples samples.
    // Expect all zero values.
    // If base_capture_ts is -1, expect zero CTS, otherwise expect
    // CTS = base_capture_ts + sample offset.
    void read_zero_samples(size_t num_samples,
                           const audio::SampleSpec& sample_spec,
                           core::nanoseconds_t base_capture_ts = -1) {
        core::Slice<audio::sample_t> samples = alloc_samples_(num_samples, sample_spec);
        audio::Frame frame(samples.data(), samples.size());
        CHECK(source_.read(frame));

        check_duration_(frame, sample_spec);
        check_timestamp_(frame, sample_spec, base_capture_ts);

        for (size_t n = 0; n < num_samples * sample_spec.num_channels(); n++) {
            DOUBLES_EQUAL(0.0, (double)frame.raw_samples()[n], SampleEpsilon);
        }
        abs_offset_ += num_samples;
        refresh_ts_offset_ = sample_spec.samples_per_chan_2_ns(abs_offset_);

        if (base_capture_ts > 0) {
            base_cts_ = base_capture_ts;
        }
    }

    // Read num_samples samples.
    // Don't check values.
    // If base_capture_ts is -1, expect zero CTS, otherwise expect
    // CTS = base_capture_ts + sample offset.
    void read_any_samples(size_t num_samples,
                          const audio::SampleSpec& sample_spec,
                          core::nanoseconds_t base_capture_ts = -1) {
        core::Slice<audio::sample_t> samples = alloc_samples_(num_samples, sample_spec);
        audio::Frame frame(samples.data(), samples.size());
        CHECK(source_.read(frame));

        check_duration_(frame, sample_spec);
        check_timestamp_(frame, sample_spec, base_capture_ts);

        abs_offset_ += num_samples;
        refresh_ts_offset_ = sample_spec.samples_per_chan_2_ns(abs_offset_);

        if (base_capture_ts > 0) {
            base_cts_ = base_capture_ts;
        }
    }

    // Get timestamp to be passed to refresh().
    // If base_capture_ts is -1, returns some non-zero base + sample offset,
    // otherwise returns base_capture_ts + sample offset.
    core::nanoseconds_t refresh_ts(core::nanoseconds_t base_capture_ts = -1) {
        if (base_capture_ts > 0) {
            base_cts_ = base_capture_ts;
        }

        return base_cts_ + refresh_ts_offset_;
    }

    // Get CTS that was read from last frame.
    core::nanoseconds_t last_capture_ts() const {
        return last_capture_ts_;
    }

    // Overwrite sample offset.
    // Normally it starts from zero and is incremented automatically when you read
    // samples, but here you can set it to arbitrary value.
    void set_offset(size_t offset) {
        offset_ = uint8_t(offset);
        abs_offset_ = offset;
    }

private:
    core::Slice<audio::sample_t> alloc_samples_(size_t num_samples,
                                                const audio::SampleSpec& sample_spec) {
        core::Slice<audio::sample_t> samples = buffer_factory_.new_buffer();
        CHECK(samples);

        samples.reslice(0, num_samples * sample_spec.num_channels());
        return samples;
    }

    void check_timestamp_(const audio::Frame& frame,
                          const audio::SampleSpec& sample_spec,
                          core::nanoseconds_t base_ts) {
        last_capture_ts_ = frame.capture_timestamp();

        if (base_ts < 0) {
            LONGS_EQUAL(0, frame.capture_timestamp());
        } else {
            const core::nanoseconds_t capture_ts =
                base_ts + sample_spec.samples_per_chan_2_ns(abs_offset_);

            expect_capture_timestamp(capture_ts, frame.capture_timestamp(), sample_spec,
                                     TimestampEpsilonSmpls);
        }
    }

    void check_duration_(const audio::Frame& frame,
                         const audio::SampleSpec& sample_spec) {
        CHECK_EQUAL(frame.num_raw_samples() / sample_spec.num_channels(),
                    frame.duration());
    }

    sndio::ISource& source_;
    core::BufferFactory<audio::sample_t>& buffer_factory_;

    uint8_t offset_;
    size_t abs_offset_;

    core::nanoseconds_t base_cts_;
    core::nanoseconds_t refresh_ts_offset_;
    core::nanoseconds_t last_capture_ts_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_FRAME_READER_H_
