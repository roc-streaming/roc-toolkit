/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <CppUTest/TestHarness.h>

#include "roc_audio/sample_spec.h"

namespace roc {
namespace audio {

TEST_GROUP(sample_spec) {};

TEST(sample_spec, num_channels) {
    SampleSpec sample_spec = SampleSpec();
    sample_spec.setChannels(0x0);
    LONGS_EQUAL(0, sample_spec.num_channels());

    sample_spec.setChannels(0x1);
    LONGS_EQUAL(1, sample_spec.num_channels());
    sample_spec.setChannels(0x2);
    LONGS_EQUAL(1, sample_spec.num_channels());
    sample_spec.setChannels(0x4);
    LONGS_EQUAL(1, sample_spec.num_channels());

    sample_spec.setChannels(0x1 | 0x2);
    LONGS_EQUAL(2, sample_spec.num_channels());
    sample_spec.setChannels(0x1 | 0x4);
    LONGS_EQUAL(2, sample_spec.num_channels());
    sample_spec.setChannels(0x2 | 0x4);
    LONGS_EQUAL(2, sample_spec.num_channels());

    sample_spec.setChannels(0x1 | 0x2 | 0x4);
    LONGS_EQUAL(3, sample_spec.num_channels());
}

} // namespace audio
} // namespace roc