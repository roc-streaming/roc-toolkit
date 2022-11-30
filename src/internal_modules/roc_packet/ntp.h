/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ntp.h
//! @brief Utitilies for NTP timestamp.

#ifndef ROC_PACKET_NTP_H_
#define ROC_PACKET_NTP_H_

#include <errno.h>
#include <sys/time.h>
#include <time.h>

#include "roc_core/errno_to_str.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! Get current NTP time.
packet::ntp_timestamp_t ntp_timestamp();

//! Compares a and b if they close enough.
bool ntp_equal_delta(ntp_timestamp_t a, ntp_timestamp_t b, ntp_timestamp_t delta);

//! Converts nanoseconds to ntp timestamp.
//! @param ns must be non-negative.
//! @remark
//! Warning: for deltas only, since NTP epoch represnted in nanoseconds could not fit
//! into 64 bits.
ntp_timestamp_t nanoseconds_2_ntp(core::nanoseconds_t ns);

//! Converts ntp timestamp to nanoseconds.
//! @remark
//! Warning: for deltas only, since NTP epoch represented in nanoseconds could not fit
//! into 64 bits.
core::nanoseconds_t ntp_2_nanoseconds(ntp_timestamp_t ts);

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_NTP_H_
