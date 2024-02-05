/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/packet_counter.h
//! @brief Packet counter.

#ifndef ROC_RTCP_PACKET_COUNTER_H_
#define ROC_RTCP_PACKET_COUNTER_H_

#include "roc_core/stddefs.h"

namespace roc {
namespace rtcp {

//! Computes number of packets in interval.
class PacketCounter {
public:
    //! Initialize.
    PacketCounter();

    //! Update and return packet counter.
    //! @p begin defines interval beginning.
    //! @p end defines interval end (exclusive).
    //! Packet counter is computes as the maximum seen distance from begin to end.
    //! If begin changes, the maximum is cleared.
    //! If end wraps around 32-bit boundary, this is taken into account.
    uint64_t update(uint32_t begin, uint32_t end);

private:
    bool first_update_;

    uint64_t begin64_;
    uint64_t end64_hi_;
    uint32_t end64_lo_;

    uint64_t counter_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_PACKET_COUNTER_H_
