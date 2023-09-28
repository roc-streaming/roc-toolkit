/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_core/time.h
//! @brief Time definitions.

#ifndef ROC_CORE_TIME_H_
#define ROC_CORE_TIME_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace core {

//! Clock identifier.
enum clock_t {
    //! Virtual monotonic clock.
    //!
    //! @remarks
    //!  Starts at unspecified point of time.
    //!
    //!  When platform supports it, uses the clock source that grows monotonically. It
    //!  never jumps backwards and is not affected by system clock change.
    //!
    //!  This clock is still subject to clock *rate* adjustments applied by NTP daemon.
    //!  When it performs synchronization, it may slightly speed up or slow down both
    //!  unix and monotonic clocks for a while.
    //!
    //!  Usually this clock is reset after reboot.
    //!  Usually this clock does not count time spent in suspended state.
    //!
    //! @note
    //!  If platform does not have monotonic clock source, unix clock is used.
    //!  Actual precision is platform-dependent.
    ClockMonotonic,

    //! Real-time Unix-time UTC clock.
    //!
    //! @remarks
    //!  Starts at 1 Jan 1970 00:00:00 UTC.
    //!
    //!  May instantly jump forwards or backwards when system administrator sets time.
    //!  May speed up or slow down when NTP daemon adjusts clock rate.
    //!  May experience discontinuities when NTP daemon inserts leap seconds.
    //!
    //! @note
    //!  Available on all platforms.
    //!  Actual precision is platform-dependent.
    ClockUnix
};

//! Nanoseconds.
typedef int64_t nanoseconds_t;

//! One nanosecond represented in nanoseconds.
const nanoseconds_t Nanosecond = 1;

//! One microsecond represented in nanoseconds.
const nanoseconds_t Microsecond = 1000 * Nanosecond;

//! One millisecond represented in nanoseconds.
const nanoseconds_t Millisecond = 1000 * Microsecond;

//! One second represented in nanoseconds.
const nanoseconds_t Second = 1000 * Millisecond;

//! One minute represented in nanoseconds.
const nanoseconds_t Minute = 60 * Second;

//! One hour represented in nanoseconds.
const nanoseconds_t Hour = 60 * Minute;

//! One day represented in nanoseconds.
const nanoseconds_t Day = 24 * Hour;

//! Get current timestamp in nanoseconds.
nanoseconds_t timestamp(clock_t clock);

//! Sleep until the specified absolute time point has been reached.
//! @remarks
//!  @p timestamp specifies absolute time point in nanoseconds.
void sleep_until(clock_t clock, nanoseconds_t timestamp);

//! Sleep specified amount of time.
//! @remarks
//!  @p duration specifies number of nanoseconds to sleep.
void sleep_for(clock_t clock, nanoseconds_t duration);

//! Convert timestamp in nanoseconds format to broken-down time.
//! @note
//!  std::tm has precision of one second.
std::tm nanoseconds_2_tm(nanoseconds_t timestamp);

//! Convert timestamp from broken-down time to nanoseconds format.
//! @note
//!  std::tm has precision of one second.
nanoseconds_t tm_2_nanoseconds(std::tm tm);

//! Compares a and b if they close enough.
bool ns_equal_delta(nanoseconds_t a, nanoseconds_t b, nanoseconds_t delta);

} // namespace core
} // namespace roc

#endif // ROC_CORE_TIME_H_
