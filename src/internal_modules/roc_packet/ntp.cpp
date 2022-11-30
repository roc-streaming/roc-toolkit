/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/ntp.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"

namespace roc {
namespace packet {

packet::ntp_timestamp_t ntp_timestamp() {
    // # of nanoseconds since unix epoch (1 Jan 1970)
    const core::nanoseconds_t ns_unix = core::timestamp(core::ClockUnix);

    // # of nanoseconds since ntp epoch (1 Jan 1900)
    // we add 70 years plus 17 leap days
    const core::nanoseconds_t ns_ntp = ns_unix + (70 * 365 + 17) * core::Day;

    return nanoseconds_2_ntp(ns_ntp);
}

bool ntp_equal_delta(ntp_timestamp_t a, ntp_timestamp_t b, ntp_timestamp_t delta) {
    packet::ntp_timestamp_t abs_error = std::max(a, b) - std::min(a, b);
    return abs_error <= delta;
}

ntp_timestamp_t nanoseconds_2_ntp(core::nanoseconds_t ns) {
    roc_panic_if_msg(ns < 0,
                     "ntp_2_nanoseconds:"
                     " can not convert negative timestamp to ntp timestamp");

    ntp_timestamp_t res_h = 0;
    ntp_timestamp_t res_l = 0;
    const uint64_t seconds = (uint64_t)(ns / core::Second);
    res_h |= seconds << 32;
    res_l |= ((((uint64_t)ns - seconds * core::Second) << 32) / (core::Second))
        & (((uint64_t)1 << 32) - 1);

    return res_h + res_l;
}

core::nanoseconds_t ntp_2_nanoseconds(ntp_timestamp_t ts) {
    core::nanoseconds_t seconds;
    core::nanoseconds_t nans;

    seconds = (core::nanoseconds_t)(ts >> 32);
    nans = (core::nanoseconds_t)((ts & (((uint64_t)1 << 32) - 1)) * core::Second);
    nans >>= 32;

    return seconds * core::Second + nans;
}

} // namespace packet
} // namespace roc
