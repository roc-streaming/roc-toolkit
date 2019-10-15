/*
 * Copyright (c) 2015 Roc authors
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

//! Convert nanoseconds to number of samples.
inline timestamp_diff_t timestamp_from_ns(core::nanoseconds_t ns, size_t sample_rate) {
    return timestamp_diff_t(roundf(float(ns) / core::Second * sample_rate));
}

//! Convert number of samples to nanoseconds.
inline core::nanoseconds_t timestamp_to_ns(timestamp_diff_t ts, size_t sample_rate) {
    return core::nanoseconds_t(roundf(float(ts) / sample_rate * core::Second));
}

//! Bitmask of channels present in audio packet.
typedef uint32_t channel_mask_t;

//! Get number of channels in mask.
static inline size_t num_channels(channel_mask_t ch_mask) {
    size_t n_ch = 0;
    for (; ch_mask != 0; ch_mask >>= 1) {
        if (ch_mask & 1) {
            n_ch++;
        }
    }
    return n_ch;
}

//! Convert frame length to frame size.
inline size_t ns_to_size(roc::core::nanoseconds_t frame_length,
                         size_t sample_rate,
                         roc::packet::channel_mask_t ch_mask) {
    return (size_t)timestamp_from_ns(frame_length, sample_rate) * num_channels(ch_mask);
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

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_UNITS_H_
