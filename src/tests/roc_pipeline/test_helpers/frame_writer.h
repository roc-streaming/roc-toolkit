/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_
#define ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_audio/frame_factory.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_sndio/isink.h"

namespace roc {
namespace pipeline {
namespace test {

// Generate audio frames and write to sink.
class FrameWriter : public core::NonCopyable<> {
public:
    FrameWriter(sndio::ISink& sink, audio::FrameFactory& frame_factory)
        : sink_(sink)
        , frame_factory_(frame_factory)
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

    // Write num_samples samples.
    // If base_capture_ts is -1, set CTS to zero.
    // Otherwise, set CTS to base_capture_ts + sample offset.
    void write_samples(size_t samples_per_chan,
                       const audio::SampleSpec& sample_spec,
                       core::nanoseconds_t base_capture_ts = -1) {
        audio::FramePtr frame = frame_factory_.allocate_frame(
            samples_per_chan * sample_spec.num_channels() * sizeof(audio::sample_t));
        CHECK(frame);

        frame->set_raw(true);
        frame->set_duration((packet::stream_timestamp_t)samples_per_chan);

        for (size_t ns = 0; ns < samples_per_chan; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                frame->raw_samples()[ns * sample_spec.num_channels() + nc] =
                    nth_sample(offset_);
            }
            offset_++;
        }

        if (base_capture_ts >= 0) {
            last_capture_ts_ =
                base_capture_ts + sample_spec.samples_per_chan_2_ns(abs_offset_);

            frame->set_capture_timestamp(last_capture_ts_);
        }

        // self-check
        sample_spec.validate_frame(*frame);

        LONGS_EQUAL(status::StatusOK, sink_.write(*frame));

        refresh_ts_offset_ = sample_spec.samples_per_chan_2_ns(abs_offset_);
        abs_offset_ += samples_per_chan;

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

    // Get CTS that was set for last written frame.
    core::nanoseconds_t last_capture_ts() const {
        return last_capture_ts_;
    }

private:
    sndio::ISink& sink_;
    audio::FrameFactory& frame_factory_;

    uint8_t offset_;
    size_t abs_offset_;

    core::nanoseconds_t base_cts_;
    core::nanoseconds_t refresh_ts_offset_;
    core::nanoseconds_t last_capture_ts_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_FRAME_WRITER_H_
