/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/ntp.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"

namespace roc {
namespace packet {

namespace {

// Number of seconds fron NTP epoch (1900-01-01) to Unix epoch (1970-01-01).
// Equals to 70 years + 17 leap days.
static uint64_t unix_epoch = uint64_t(70 * 365 + 17) * (24 * 60 * 60);

} // namespace

bool ntp_equal_delta(ntp_timestamp_t a, ntp_timestamp_t b, ntp_timestamp_t delta) {
    ntp_timestamp_t abs_error = std::max(a, b) - std::min(a, b);
    return abs_error <= delta;
}

ntp_timestamp_t unix_2_ntp(core::nanoseconds_t unix_time) {
    const core::nanoseconds_t ntp_time =
        unix_time + (core::nanoseconds_t)unix_epoch * core::Second;

    return nanoseconds_2_ntp(ntp_time);
}

core::nanoseconds_t ntp_2_unix(ntp_timestamp_t ntp_time) {
    const ntp_timestamp_t unix_epoch_ntp = (unix_epoch << 32);

    if (ntp_time < unix_epoch_ntp) {
        return -ntp_2_nanoseconds(unix_epoch_ntp - ntp_time);
    }

    return ntp_2_nanoseconds(ntp_time - unix_epoch_ntp);
}

ntp_timestamp_t nanoseconds_2_ntp(core::nanoseconds_t ns_delta) {
    roc_panic_if_msg(ns_delta < 0, "ntp: can not convert negative delta to ntp");

    ntp_timestamp_t res_h = 0;
    ntp_timestamp_t res_l = 0;
    const uint64_t seconds = (uint64_t)(ns_delta / core::Second);
    res_h |= seconds << 32;
    res_l |= ((((uint64_t)ns_delta - seconds * core::Second) << 32) / (core::Second))
        & (((uint64_t)1 << 32) - 1);

    return res_h + res_l;
}

core::nanoseconds_t ntp_2_nanoseconds(ntp_timestamp_t ntp_delta) {
    core::nanoseconds_t seconds;
    core::nanoseconds_t nans;

    seconds = (core::nanoseconds_t)(ntp_delta >> 32);
    nans = (core::nanoseconds_t)((ntp_delta & (((uint64_t)1 << 32) - 1)) * core::Second);
    nans >>= 32;

    return seconds * core::Second + nans;
}

} // namespace packet
} // namespace roc
