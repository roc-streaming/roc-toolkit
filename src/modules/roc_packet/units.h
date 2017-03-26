/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/units.h
//! @brief Typedefs for various units.

#ifndef ROC_PACKET_UNITS_H_
#define ROC_PACKET_UNITS_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace packet {

//! Packet source ID identifying client stream.
typedef uint32_t source_t;

//! Packet sequence number in client stream.
typedef uint16_t seqnum_t;

//! Packet sequence number (signed version).
typedef int16_t signed_seqnum_t;

//! Audio packet timestamp (number of first sample in packet).
typedef uint32_t timestamp_t;

//! Audio packet timestamp (signed version).
typedef int32_t signed_timestamp_t;

//! Bitmask of channels present in audio packet.
typedef uint32_t channel_mask_t;

//! Channel number.
typedef uint16_t channel_t;

//! Audio sample.
typedef float sample_t;

//! Maximum possible value of a sample.
extern const sample_t sample_max_val;

//! Minimum possible value of a sample.
extern const sample_t sample_min_val;

//! Get number of channels in mask.
static inline size_t num_channels(channel_mask_t ch_mask) {
    size_t n_ch = 0;
    for (; ch_mask != 0; ch_mask >>= 1) {
        n_ch++;
    }
    return n_ch;
}

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_UNITS_H_
