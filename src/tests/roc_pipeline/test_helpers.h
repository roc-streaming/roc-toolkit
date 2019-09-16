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
#include "roc_packet/address.h"

namespace roc {
namespace pipeline {

namespace {

const double Epsilon = 0.00001;

inline audio::sample_t nth_sample(uint8_t n) {
    return audio::sample_t(n) / 1024;
}

inline packet::Address new_address(int port) {
    packet::Address addr;
    CHECK(addr.set_host_ipv4("127.0.0.1", port));
    return addr;
}

} // namespace

} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_H_
