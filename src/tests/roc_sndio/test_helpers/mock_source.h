/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_SNDIO_TEST_HELPERS_MOCK_SOURCE_H_
#define ROC_SNDIO_TEST_HELPERS_MOCK_SOURCE_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/frame_factory.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/time.h"
#include "roc_sndio/isource.h"

namespace roc {
namespace sndio {
namespace test {

class MockSource : public ISource {
public:
    MockSource(audio::FrameFactory& frame_factory, const audio::SampleSpec& sample_spec)
        : frame_factory_(frame_factory)
        , sample_spec_(sample_spec)
        , pos_(0)
        , size_(0) {
    }

    virtual DeviceType type() const {
        return DeviceType_Source;
    }

    virtual ISink* to_sink() {
        return NULL;
    }

    virtual ISource* to_source() {
        return this;
    }

    virtual audio::SampleSpec sample_spec() const {
        return audio::SampleSpec();
    }

    virtual bool has_state() const {
        return true;
    }

    virtual DeviceState state() const {
        if (pos_ >= size_) {
            return DeviceState_Idle;
        } else {
            return DeviceState_Active;
        }
    }

    virtual status::StatusCode pause() {
        FAIL("not implemented");
        return status::StatusAbort;
    }

    virtual status::StatusCode resume() {
        FAIL("not implemented");
        return status::StatusAbort;
    }

    virtual bool has_latency() const {
        return false;
    }

    virtual bool has_clock() const {
        return false;
    }

    virtual status::StatusCode rewind() {
        FAIL("not implemented");
        return status::StatusAbort;
    }

    virtual void reclock(core::nanoseconds_t) {
        // no-op
    }

    virtual status::StatusCode read(audio::Frame& frame,
                                    packet::stream_timestamp_t duration,
                                    audio::FrameReadMode mode) {
        LONGS_EQUAL(audio::ModeHard, mode);

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

        return status::StatusOK;
    }

    void add(size_t sz) {
        CHECK(size_ + sz <= MaxSz);

        for (size_t n = 0; n < sz; n++) {
            samples_[size_] = nth_sample_(size_);
            size_++;
        }
    }

    size_t num_returned() const {
        return pos_;
    }

private:
    enum { MaxSz = 256 * 1024 };

    audio::sample_t nth_sample_(size_t n) {
        return audio::sample_t(uint8_t(n)) / audio::sample_t(1 << 8);
    }

    audio::FrameFactory& frame_factory_;
    const audio::SampleSpec sample_spec_;

    audio::sample_t samples_[MaxSz];
    size_t pos_;
    size_t size_;
};

} // namespace test
} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_TEST_HELPERS_MOCK_SOURCE_H_
