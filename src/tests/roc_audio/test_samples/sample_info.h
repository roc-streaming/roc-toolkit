/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_AUDIO_TEST_SAMPLES_SAMPLE_INFO_H_
#define ROC_AUDIO_TEST_SAMPLES_SAMPLE_INFO_H_

#include "roc_audio/pcm_subformat.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {
namespace test {

struct SampleInfo {
    enum { MaxSamples = 50000, MaxBytes = 500000 };

    const char* name;

    PcmSubformat format;

    size_t num_samples;
    float samples[MaxSamples];

    size_t num_bytes;
    uint8_t bytes[MaxBytes];
};

} // namespace test
} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_TEST_SAMPLES_SAMPLE_INFO_H_
