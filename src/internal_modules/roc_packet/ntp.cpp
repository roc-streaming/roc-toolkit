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

// Number of seconds from NTP epoch (1900-01-01) to Unix epoch (1970-01-01).
// Equals to 70 years with 17 leap days.
static uint64_t UnixEpoch = uint64_t(70 * 365 + 17) * (24 * 3600);

// Number of seconds from Unix epoch to NTP Era 1 (7 Feb 2036 6:28:16).
// Equals to 66 years with 16 leap days, 37 days, 6 hours, 28 minutes and 16 seconds.
// See RFC 2030.
static uint64_t Era1 =
    uint64_t(66 * 365 + 16 + 37) * (24 * 3600) + (6 * 3600) + (28 * 60) + 16;

uint64_t bits_pack(uint32_t hi, uint32_t lo) {
    return ((uint64_t)hi << 32) | (uint64_t)lo;
}

void bits_unpack(uint64_t value, uint32_t& hi, uint32_t& lo) {
    hi = uint32_t((value & 0xffffffff00000000ull) >> 32);
    lo = uint32_t(value & 0xffffffffull);
}

} // namespace

ntp_timestamp_t unix_2_ntp(core::nanoseconds_t unix_time) {
    if (unix_time < (core::nanoseconds_t)Era1 * core::Second) {
        // "normal" ntp (1968-2036)
        const core::nanoseconds_t ntp_time =
            unix_time + (core::nanoseconds_t)UnixEpoch * core::Second;
        return nanoseconds_2_ntp(ntp_time);
    }

    // "era1" ntp (2036-2104)
    const core::nanoseconds_t ntp_time =
        unix_time - (core::nanoseconds_t)Era1 * core::Second;
    return nanoseconds_2_ntp(ntp_time);
}

core::nanoseconds_t ntp_2_unix(ntp_timestamp_t ntp_time) {
    if (ntp_time & 0x8000000000000000ull) {
        // "normal" ntp (1968-2036)
        const ntp_timestamp_t unix_epoch_ntp = (UnixEpoch << 32);

        if (ntp_time < unix_epoch_ntp) {
            // negative unix time (1968-1970)
            return -ntp_2_nanoseconds(unix_epoch_ntp - ntp_time);
        }
        return ntp_2_nanoseconds(ntp_time - unix_epoch_ntp);
    }

    // "era1" ntp (2036-2104)
    const ntp_timestamp_t era1_ntp = (Era1 << 32);
    return ntp_2_nanoseconds(ntp_time + era1_ntp);
}

ntp_timestamp_t nanoseconds_2_ntp(core::nanoseconds_t ns_delta) {
    roc_panic_if_msg(ns_delta < 0, "ntp: can not convert negative delta to ntp");

    const uint64_t seconds = (uint64_t)ns_delta / core::Second;
    const uint64_t nans = (uint64_t)ns_delta - (seconds * core::Second);
    const uint64_t fraction = ((uint64_t)nans << 32) / core::Second;

    return bits_pack((uint32_t)seconds, (uint32_t)fraction);
}

core::nanoseconds_t ntp_2_nanoseconds(ntp_timestamp_t ntp_delta) {
    uint32_t hi = 0;
    uint32_t lo = 0;
    bits_unpack(ntp_delta, hi, lo);

    const core::nanoseconds_t seconds = (core::nanoseconds_t)hi;
    const core::nanoseconds_t nans = (core::nanoseconds_t)(lo * core::Second) >> 32;

    return seconds * core::Second + nans;
}

} // namespace packet
} // namespace roc
