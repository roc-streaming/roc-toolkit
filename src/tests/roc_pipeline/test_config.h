/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_CONFIG_H_
#define ROC_PIPELINE_TEST_CONFIG_H_

#include "roc_config/config.h"

namespace roc {
namespace test {

enum {
    // Number of channels enabled.
    NumChannels = 2,

    // Mask of enabled channels.
    ChannelMask = 0x3,

    // Sample wrap.
    MaxSampleValue = 10000,

    // Sample rate.
    SampleRate = ROC_CONFIG_DEFAULT_SAMPLE_RATE
};

} // namespace test
} // namespace roc

#endif // ROC_PIPELINE_TEST_CONFIG_H_
