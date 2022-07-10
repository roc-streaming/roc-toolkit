/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/units.h
//! @brief Various units used in packets.

#ifndef ROC_PACKET_UNITS_H_
#define ROC_PACKET_UNITS_H_

#include "roc_core/stddefs.h"
#include "roc_core/time.h"

namespace roc {
namespace packet {

//! Packet source ID identifying packet stream.
typedef uint32_t source_t;

//! Audio packet timestamp.
typedef uint32_t timestamp_t;

//! Audio packet timestamps difference.
typedef int32_t timestamp_diff_t;

//! Compute difference between two timestamps.
inline timestamp_diff_t timestamp_diff(timestamp_t a, timestamp_t b) {
    return timestamp_diff_t(a - b);
}

//! Check if a is before b taking possible wrap into account.
inline bool timestamp_lt(timestamp_t a, timestamp_t b) {
    return timestamp_diff(a, b) < 0;
}

//! Check if a is before or equal to b taking possible wrap into account.
inline bool timestamp_le(timestamp_t a, timestamp_t b) {
    return timestamp_diff(a, b) <= 0;
}

//! Packet sequence number in packet stream.
typedef uint16_t seqnum_t;

//! Packet sequence numbers difference.
typedef int16_t seqnum_diff_t;

//! Compute difference between two seqnums.
inline seqnum_diff_t seqnum_diff(seqnum_t a, seqnum_t b) {
    return seqnum_diff_t(a - b);
}

//! Check if a is before b taking possible wrap into account.
inline bool seqnum_lt(seqnum_t a, seqnum_t b) {
    return seqnum_diff(a, b) < 0;
}

//! Check if a is before or equal to b taking possible wrap into account.
inline bool seqnum_le(seqnum_t a, seqnum_t b) {
    return seqnum_diff(a, b) <= 0;
}

//! FEC block number in a packet stream.
typedef uint16_t blknum_t;

//! FEC block numbers difference.
typedef int16_t blknum_diff_t;

//! Compute difference between two FEC block numbers.
inline blknum_diff_t blknum_diff(blknum_t a, blknum_t b) {
    return blknum_diff_t(a - b);
}

//! Check if a is before b taking possible wrap into account.
inline bool blknum_lt(blknum_t a, blknum_t b) {
    return blknum_diff(a, b) < 0;
}

//! Check if a is before or equal to b taking possible wrap into account.
inline bool blknum_le(blknum_t a, blknum_t b) {
    return blknum_diff(a, b) <= 0;
}

//! NTP timestamp.
//! @remarks
//!  Highest 32 bits - seconds since NTP epoch, lowest 32 bits - fractions of a second.
//!  NTP epoch starts from January 1, 1900.
typedef uint64_t ntp_timestamp_t;

//! Bitmask of channels present in audio packet.
typedef uint32_t channel_mask_t;

//! Compute number of channels in mask.
inline size_t num_channels(channel_mask_t ch_mask) {
    size_t n_ch = 0;
    for (; ch_mask != 0; ch_mask >>= 1) {
        if (ch_mask & 1) {
            n_ch++;
        }
    }
    return n_ch;
}

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_UNITS_H_
