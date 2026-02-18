/*
 * Copyright (c) 2020 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_
#define ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_

#include <CppUTest/TestHarness.h>

#include "test_helpers/utils.h"

#include "roc_audio/frame_factory.h"
#include "roc_audio/sample_spec.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace pipeline {
namespace test {

class MockSource : public sndio::ISource {
public:
    MockSource(audio::FrameFactory& frame_factory,
               const audio::SampleSpec& sample_spec,
               core::IArena& arena)
        : sndio::IDevice(arena)
        , sndio::ISource(arena)
        , frame_factory_(frame_factory)
        , sample_spec_(sample_spec)
        , state_(sndio::DeviceState_Active)
        , pos_(0)
        , size_(0)
        , value_(0)
        , n_ch_(0) {
    }

    virtual sndio::DeviceType type() const {
        return sndio::DeviceType_Source;
    }

    virtual sndio::ISink* to_sink() {
        return NULL;
    }

    virtual sndio::ISource* to_source() {
        return this;
    }

    virtual audio::SampleSpec sample_spec() const {
        return audio::SampleSpec();
    }

    core::nanoseconds_t frame_length() const {
        return 0;
    }

    virtual bool has_state() const {
        return true;
    }

    virtual sndio::DeviceState state() const {
        return state_;
    }

    void set_state(sndio::DeviceState st) {
        state_ = st;
    }

    virtual status::StatusCode pause() {
        state_ = sndio::DeviceState_Paused;
        return status::StatusOK;
    }

    virtual status::StatusCode resume() {
        state_ = sndio::DeviceState_Active;
        return status::StatusOK;
    }

    virtual bool has_latency() const {
        return false;
    }

    virtual bool has_clock() const {
        return false;
    }

    virtual status::StatusCode rewind() {
        state_ = sndio::DeviceState_Active;
        return status::StatusOK;
    }

    virtual void reclock(core::nanoseconds_t) {
        // no-op
    }

    virtual status::StatusCode read(audio::Frame& frame,
                                    packet::stream_timestamp_t duration,
                                    audio::FrameReadMode mode) {
        LONGS_EQUAL(audio::ModeHard, mode);

        if (pos_ == size_) {
            return status::StatusFinish;
        }

        CHECK(frame_factory_.reallocate_frame(
            frame, sample_spec_.stream_timestamp_2_bytes(duration)));

        frame.set_raw(true);
        frame.set_duration(duration);

        size_t ns = frame.num_raw_samples();
        if (ns > size_ - pos_) {
            ns = size_ - pos_;
        }

        if (ns > 0) {
            memcpy(frame.raw_samples(), samples_ + pos_, ns * sizeof(audio::sample_t));
            pos_ += ns;
        }

        if (ns < frame.num_raw_samples()) {
            memset(frame.raw_samples() + ns, 0,
                   (frame.num_raw_samples() - ns) * sizeof(audio::sample_t));
        }

        CHECK(n_ch_ > 0);
        frame.set_duration(frame.num_raw_samples() / n_ch_);

        return status::StatusOK;
    }

    virtual status::StatusCode close() {
        return status::StatusOK;
    }

    virtual void dispose() {
        arena().dispose_object(*this);
    }

    void add(size_t num_samples, const audio::SampleSpec& sample_spec) {
        CHECK(size_ + num_samples * sample_spec.num_channels() <= MaxSz);

        for (size_t ns = 0; ns < num_samples; ns++) {
            for (size_t nc = 0; nc < sample_spec.num_channels(); nc++) {
                samples_[size_] = nth_sample((uint8_t)value_);
                size_++;
            }
            value_++;
        }

        if (!n_ch_) {
            n_ch_ = sample_spec.num_channels();
        }
        LONGS_EQUAL(n_ch_, sample_spec.num_channels());
    }

    size_t num_remaining() const {
        return size_ - pos_;
    }

private:
    enum { MaxSz = 256 * 1024 };

    audio::FrameFactory& frame_factory_;
    const audio::SampleSpec sample_spec_;

    sndio::DeviceState state_;

    audio::sample_t samples_[MaxSz];

    size_t pos_;
    size_t size_;
    size_t value_;
    size_t n_ch_;
};

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_MOCK_SOURCE_H_
