/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef ROC_PIPELINE_TEST_HELPERS_UTILS_H_
#define ROC_PIPELINE_TEST_HELPERS_UTILS_H_

#include <CppUTest/TestHarness.h>

#include "roc_address/socket_addr.h"
#include "roc_audio/sample.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_pipeline/config.h"

namespace roc {
namespace pipeline {
namespace test {

namespace {

const double SampleEpsilon = 0.00001;

const core::nanoseconds_t TimestampEpsilon =
    (core::nanoseconds_t)(1. / DefaultSampleRate * core::Second);

inline audio::sample_t nth_sample(uint8_t n) {
    return audio::sample_t(n) / 1024;
}

inline address::SocketAddr new_address(int port) {
    address::SocketAddr addr;
    CHECK(addr.set_host_port(address::Family_IPv4, "127.0.0.1", port));
    return addr;
}

inline void expect_capture_timestamp(core::nanoseconds_t expected,
                                     core::nanoseconds_t actual,
                                     core::nanoseconds_t epsilon) {
    if (!core::ns_equal_delta(expected, actual, epsilon)) {
        char sbuff[256];
        snprintf(sbuff, sizeof(sbuff),
                 "failed comparing capture timestamps:\n"
                 " expected:  %lld\n"
                 " actual:    %lld\n"
                 " delta:     %lld\n"
                 " max_delta: %lld\n",
                 (long long)expected, (long long)actual, (long long)(expected - actual),
                 (long long)epsilon);
        FAIL(sbuff);
    }
}

} // namespace

} // namespace test
} // namespace pipeline
} // namespace roc

#endif // ROC_PIPELINE_TEST_HELPERS_UTILS_H_
