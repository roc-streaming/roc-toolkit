/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_SNDIO_TEST_HELPERS_MOCK_SINK_H_
#define ROC_SNDIO_TEST_HELPERS_MOCK_SINK_H_

#include <CppUTest/TestHarness.h>

#include "roc_sndio/isink.h"

namespace roc {
namespace sndio {
namespace test {

class MockSink : public ISink {
public:
    MockSink(core::IArena& arena)
        : IDevice(arena)
        , ISink(arena)
        , pos_(0) {
    }

    virtual DeviceType type() const {
        return DeviceType_Sink;
    }

    virtual ISink* to_sink() {
        return this;
    }

    virtual ISource* to_source() {
        return NULL;
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

    virtual DeviceState state() const {
        return DeviceState_Active;
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

    virtual status::StatusCode write(audio::Frame& frame) {
        CHECK(pos_ + frame.num_raw_samples() <= MaxSz);

        memcpy(samples_ + pos_, frame.raw_samples(),
               frame.num_raw_samples() * sizeof(audio::sample_t));
        pos_ += frame.num_raw_samples();
        return status::StatusOK;
    }

    virtual ROC_ATTR_NODISCARD status::StatusCode flush() {
        return status::StatusOK;
    }

    virtual status::StatusCode close() {
        return status::StatusOK;
    }

    virtual void dispose() {
        arena().dispose_object(*this);
    }

    void check(size_t offset, size_t size) {
        UNSIGNED_LONGS_EQUAL(pos_, size);

        for (size_t n = 0; n < size; n++) {
            DOUBLES_EQUAL((double)samples_[n], (double)nth_sample_(offset + n), 0.0001);
        }
    }

private:
    enum { MaxSz = 256 * 1024 };

    audio::sample_t nth_sample_(size_t n) {
        return audio::sample_t(uint8_t(n)) / audio::sample_t(1 << 8);
    }

    audio::sample_t samples_[MaxSz];
    size_t pos_;
};

} // namespace test
} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_TEST_HELPERS_MOCK_SINK_H_
