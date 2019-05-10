/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_H_
#define ROC_PIPELINE_TEST_HELPERS_H_

#include <CppUTest/TestHarness.h>

#include <stdio.h>

#include "roc_audio/units.h"
#include "roc_core/stddefs.h"
#include "roc_packet/parse_address.h"

namespace roc {
namespace pipeline {

namespace {

const audio::sample_t Epsilon = 0.00001f;

inline audio::sample_t nth_sample(uint8_t n) {
    return audio::sample_t(n) / 1024;
}

inline packet::Address new_address(int port) {
    char str[64];
    snprintf(str, sizeof(str), "127.0.0.1:%d", port);

    packet::Address addr;
    CHECK(packet::parse_address(str, addr));

    return addr;
}

} // namespace

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_H_
