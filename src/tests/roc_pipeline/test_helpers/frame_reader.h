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

#include "roc_audio/frame_factory.h"
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
    FrameReader(sndio::ISource& source, audio::FrameFactory& frame_factory)
        : source_(source)
        , frame_factory_(frame_factory)
        , sample_offset_(0)
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
        audio::FramePtr frame =
            read_frame_(status::StatusOK, num_samples, sample_spec, audio::ModeHard);

        check_duration_(*frame, num_samples, sample_spec);
        check_timestamp_(*frame, sample_spec, base_capture_ts);

        for (size_t ns = 0; ns < num_samples; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                DOUBLES_EQUAL(
                    (double)nth_sample(sample_offset_) * num_sessions,
                    (double)frame->raw_samples()[ns * sample_spec.num_channels() + nc],
                    SampleEpsilon);
            }
            sample_offset_++;
        }

        advance_(num_samples, sample_spec, base_capture_ts);
    }

    // Read num_samples samples.
    // Expect any non-zero values.
    // If base_capture_ts is -1, expect zero CTS, otherwise expect
    // CTS = base_capture_ts + sample offset.
    void read_nonzero_samples(size_t num_samples,
                              const audio::SampleSpec& sample_spec,
                              core::nanoseconds_t base_capture_ts = -1) {
        audio::FramePtr frame =
            read_frame_(status::StatusOK, num_samples, sample_spec, audio::ModeHard);

        check_duration_(*frame, num_samples, sample_spec);
        check_timestamp_(*frame, sample_spec, base_capture_ts);

        size_t non_zero = 0;
        for (size_t ns = 0; ns < num_samples * sample_spec.num_channels(); ns++) {
            if (frame->raw_samples()[ns] != 0) {
                non_zero++;
            }
        }
        CHECK(non_zero > 0);

        sample_offset_ = uint8_t(sample_offset_ + num_samples);

        advance_(num_samples, sample_spec, base_capture_ts);
    }

    // Read num_samples samples.
    // Expect all zero values.
    // If base_capture_ts is -1, expect zero CTS, otherwise expect
    // CTS = base_capture_ts + sample offset.
    void read_zero_samples(size_t num_samples,
                           const audio::SampleSpec& sample_spec,
                           core::nanoseconds_t base_capture_ts = -1) {
        audio::FramePtr frame =
            read_frame_(status::StatusOK, num_samples, sample_spec, audio::ModeHard);

        check_duration_(*frame, num_samples, sample_spec);
        check_timestamp_(*frame, sample_spec, base_capture_ts);

        for (size_t n = 0; n < num_samples * sample_spec.num_channels(); n++) {
            DOUBLES_EQUAL(0.0, (double)frame->raw_samples()[n], SampleEpsilon);
        }

        sample_offset_ = uint8_t(sample_offset_ + num_samples);

        advance_(num_samples, sample_spec, base_capture_ts);
    }

    // Read num_samples samples.
    // Don't check values.
    // If base_capture_ts is -1, expect zero CTS, otherwise expect
    // CTS = base_capture_ts + sample offset.
    void read_any_samples(size_t num_samples,
                          const audio::SampleSpec& sample_spec,
                          core::nanoseconds_t base_capture_ts = -1) {
        audio::FramePtr frame =
            read_frame_(status::StatusOK, num_samples, sample_spec, audio::ModeHard);

        check_duration_(*frame, num_samples, sample_spec);
        check_timestamp_(*frame, sample_spec, base_capture_ts);

        sample_offset_ = uint8_t(sample_offset_ + num_samples);

        advance_(num_samples, sample_spec, base_capture_ts);
    }

    // Int16 version of read_samples().
    void read_s16_samples(size_t num_samples,
                          size_t num_sessions,
                          const audio::SampleSpec& sample_spec,
                          core::nanoseconds_t base_capture_ts = -1) {
        audio::FramePtr frame =
            read_frame_(status::StatusOK, num_samples, sample_spec, audio::ModeHard);

        check_duration_(*frame, num_samples, sample_spec);
        check_timestamp_(*frame, sample_spec, base_capture_ts);

        UNSIGNED_LONGS_EQUAL(num_samples * sample_spec.num_channels() * sizeof(int16_t),
                             frame->num_bytes());

        const int16_t* samples = (const int16_t*)frame->bytes();

        for (size_t ns = 0; ns < num_samples; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                DOUBLES_EQUAL((double)nth_sample(sample_offset_) * num_sessions,
                              (double)samples[ns * sample_spec.num_channels() + nc]
                                  / 32768.0,
                              SampleEpsilon);
            }
            sample_offset_++;
        }

        advance_(num_samples, sample_spec, base_capture_ts);
    }

    // Int32 version of read_samples().
    void read_s32_samples(size_t num_samples,
                          size_t num_sessions,
                          const audio::SampleSpec& sample_spec,
                          core::nanoseconds_t base_capture_ts = -1) {
        audio::FramePtr frame =
            read_frame_(status::StatusOK, num_samples, sample_spec, audio::ModeHard);

        check_duration_(*frame, num_samples, sample_spec);
        check_timestamp_(*frame, sample_spec, base_capture_ts);

        UNSIGNED_LONGS_EQUAL(num_samples * sample_spec.num_channels() * sizeof(int32_t),
                             frame->num_bytes());

        const int32_t* samples = (const int32_t*)frame->bytes();

        for (size_t ns = 0; ns < num_samples; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                DOUBLES_EQUAL((double)nth_sample(sample_offset_) * num_sessions,
                              (double)samples[ns * sample_spec.num_channels() + nc]
                                  / 2147483648.0,
                              SampleEpsilon);
            }
            sample_offset_++;
        }

        advance_(num_samples, sample_spec, base_capture_ts);
    }

    // Same as read_samples(), but enables soft read mode and allows
    // StatusPart and StatusDrain.
    void read_samples_soft(size_t requested_samples,
                           size_t expected_samples,
                           size_t num_sessions,
                           const audio::SampleSpec& sample_spec,
                           core::nanoseconds_t base_capture_ts = -1) {
        CHECK(expected_samples <= requested_samples);

        audio::FramePtr frame =
            read_frame_(expected_samples == requested_samples ? status::StatusOK
                            : expected_samples > 0            ? status::StatusPart
                                                              : status::StatusDrain,
                        requested_samples, sample_spec, audio::ModeSoft);

        if (expected_samples == 0) {
            return;
        }

        check_duration_(*frame, expected_samples, sample_spec);
        check_timestamp_(*frame, sample_spec, base_capture_ts);

        for (size_t ns = 0; ns < expected_samples; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                DOUBLES_EQUAL(
                    (double)nth_sample(sample_offset_) * num_sessions,
                    (double)frame->raw_samples()[ns * sample_spec.num_channels() + nc],
                    SampleEpsilon);
            }
            sample_offset_++;
        }

        advance_(expected_samples, sample_spec, base_capture_ts);
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
        sample_offset_ = uint8_t(offset);
        abs_offset_ = offset;
    }

private:
    audio::FramePtr read_frame_(status::StatusCode expected_status,
                                size_t requested_samples,
                                const audio::SampleSpec& sample_spec,
                                audio::FrameReadMode read_mode) {
        audio::FramePtr frame = frame_factory_.allocate_frame_no_buffer();
        CHECK(frame);

        packet::stream_timestamp_t requested_duration =
            packet::stream_timestamp_t(requested_samples);

        LONGS_EQUAL(expected_status, source_.read(*frame, requested_duration, read_mode));

        if (expected_status == status::StatusDrain) {
            return NULL;
        }

        if (sample_spec.is_raw()) {
            CHECK(frame->is_raw());
            CHECK(frame->raw_samples());
        } else {
            CHECK(!frame->is_raw());
        }

        CHECK(frame->bytes());

        return frame;
    }

    void check_duration_(const audio::Frame& frame,
                         size_t expected_samples,
                         const audio::SampleSpec& sample_spec) {
        UNSIGNED_LONGS_EQUAL(expected_samples, frame.duration());

        if (sample_spec.is_raw()) {
            UNSIGNED_LONGS_EQUAL(expected_samples * sample_spec.num_channels(),
                                 frame.num_raw_samples());
        }

        UNSIGNED_LONGS_EQUAL(expected_samples,
                             sample_spec.bytes_2_stream_timestamp(frame.num_bytes()));
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

    void advance_(size_t num_samples,
                  const audio::SampleSpec& sample_spec,
                  core::nanoseconds_t base_capture_ts) {
        abs_offset_ += num_samples;
        refresh_ts_offset_ = sample_spec.samples_per_chan_2_ns(abs_offset_);

        if (base_capture_ts > 0) {
            base_cts_ = base_capture_ts;
        }
    }

    sndio::ISource& source_;
    audio::FrameFactory& frame_factory_;

    uint8_t sample_offset_;
    size_t abs_offset_;

    core::nanoseconds_t base_cts_;
    core::nanoseconds_t refresh_ts_offset_;
    core::nanoseconds_t last_capture_ts_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_FRAME_READER_H_
