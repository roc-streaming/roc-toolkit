/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_HELPERS_MOCK_READER_TS_H_
#define ROC_AUDIO_TEST_HELPERS_MOCK_READER_TS_H_

#include <CppUTest/TestHarness.h>

#include "roc_audio/iframe_reader.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "test_helpers/mock_reader.h"

namespace roc {
namespace audio {
namespace test {

class MockReaderTs : public MockReader {
public:
    explicit MockReaderTs(const core::nanoseconds_t base_timestamp,
                          SampleSpec sample_spec,
                          bool fail_on_empty = true)
        : MockReader(fail_on_empty)
        , sample_spec_(sample_spec)
        , base_timestamp_(base_timestamp) {
    }

    virtual bool read(Frame& frame) {
        if (MockReader::read(frame)) {
            frame.set_capture_timestamp(base_timestamp_);
            base_timestamp_ += sample_spec_.samples_overall_2_ns(frame.num_samples());
            return true;
        } else {
            return false;
        }
    }

private:
    SampleSpec sample_spec_;
    core::nanoseconds_t base_timestamp_;
};

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_HELPERS_MOCK_READER_TS_H_
