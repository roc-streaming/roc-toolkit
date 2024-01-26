/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/ntp.h
//! @brief RTCP-specific NTP helpers.

#ifndef ROC_RTCP_NTP_H_
#define ROC_RTCP_NTP_H_

#include "roc_packet/ntp.h"

namespace roc {
namespace rtcp {

//! Clamp 64-bit NTP timestamp so that it does not exceed maximum.
packet::ntp_timestamp_t ntp_clamp_64(packet::ntp_timestamp_t value,
                                     packet::ntp_timestamp_t max_value);

//! Clamp 64-bit NTP timestamp so that it fits into middle 32-bits.
//! Value is rounded to the new resolution and capped with given maximum.
//! Returned value has zeros in high and low 16 bits.
packet::ntp_timestamp_t ntp_clamp_32(packet::ntp_timestamp_t value,
                                     packet::ntp_timestamp_t max_value);

//! Restore full 64-bit NTP timestamp from middle 32 bits.
//! @param value is middle 32 bits of timestamp to be restored.
//! @param base is full 64 bit timestamp that was recently obtained from same source.
//! The function will combine high 16 bits of base with value.
//! It will also detect possible wrap and apply correction if needed.
packet::ntp_timestamp_t ntp_extend(packet::ntp_timestamp_t base,
                                   packet::ntp_timestamp_t value);

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_NTP_H_
