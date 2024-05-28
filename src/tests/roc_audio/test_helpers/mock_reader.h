/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_
#define ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
namespace test {

class MockReader : public IFrameReader {
public:
    MockReader(FrameFactory& frame_factory, const SampleSpec& sample_spec)
        : frame_factory_(frame_factory)
        , sample_spec_(sample_spec)
        , total_reads_(0)
        , pos_(0)
        , size_(0)
        , timestamp_(-1)
        , max_duration_(0)
        , status_(status::NoStatus)
        , last_status_(status::NoStatus) {
    }

    virtual status::StatusCode read(Frame& frame,
                                    packet::stream_timestamp_t requested_duration) {
        if (status_ != status::NoStatus && status_ != status::StatusOK) {
            return (last_status_ = status_);
        }

        packet::stream_timestamp_t duration = std::min(
            requested_duration,
            packet::stream_timestamp_t((size_ - pos_) / sample_spec_.num_channels()));

        if (max_duration_ != 0) {
            duration = std::min(duration, max_duration_);
        }

        if (duration == 0) {
            return (last_status_ = status::StatusEnd);
        }

        total_reads_++;

        CHECK(frame_factory_.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(duration)));

        frame.set_raw(true);
        frame.set_duration(duration);

        memcpy(frame.raw_samples(), samples_ + pos_,
               frame.num_raw_samples() * sizeof(sample_t));

        unsigned flags = 0;
        for (size_t n = pos_; n < pos_ + frame.num_raw_samples(); n++) {
            flags |= flags_[n];
        }
        frame.set_flags(flags);

        pos_ += frame.num_raw_samples();

        if (timestamp_ >= 0) {
            frame.set_capture_timestamp(timestamp_);
            timestamp_ += sample_spec_.samples_overall_2_ns(frame.num_raw_samples());
        }

        return (last_status_ = (duration == requested_duration ? status::StatusOK
                                                               : status::StatusPart));
    }

    void set_status(status::StatusCode status) {
        status_ = status;
    }

    void set_limit(packet::stream_timestamp_t max_duration) {
        max_duration_ = max_duration;
    }

    void enable_timestamps(const core::nanoseconds_t base_timestamp) {
        timestamp_ = base_timestamp;
    }

    void add_samples(size_t size, sample_t value, unsigned flags = 0) {
        CHECK(size_ + size < MaxSz);

        for (size_t n = 0; n < size; n++) {
            samples_[size_] = value;
            flags_[size_] = flags;
            size_++;
        }
    }

    void add_zero_samples() {
        while (size_ < MaxSz) {
            samples_[size_] = 0;
            flags_[size_] = 0;
            size_++;
        }
    }

    size_t total_reads() const {
        return total_reads_;
    }

    size_t num_unread() const {
        return size_ - pos_;
    }

    status::StatusCode last_status() const {
        return last_status_;
    }

private:
    enum { MaxSz = 100000 };

    FrameFactory& frame_factory_;
    SampleSpec sample_spec_;

    size_t total_reads_;

    sample_t samples_[MaxSz];
    unsigned flags_[MaxSz];
    size_t pos_;
    size_t size_;

    core::nanoseconds_t timestamp_;

    packet::stream_timestamp_t max_duration_;

    status::StatusCode status_;
    status::StatusCode last_status_;
};

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_MOCK_READER_H_
