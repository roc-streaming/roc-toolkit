/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/isequencer.h
//! @brief Packet sequencer.

#ifndef ROC_PACKET_ISEQUENCER_H_
#define ROC_PACKET_ISEQUENCER_H_

#include "roc_core/time.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Packet sequencer.
//! Fills protocol-specific packet headers to form a continous sequence.
//! For example, RTP sequencer fills packet seqnums and timestamps.
class ISequencer {
public:
    virtual ~ISequencer();

    //! Fill next packet.
    virtual void
    next(Packet& packet, core::nanoseconds_t capture_ts, stream_timestamp_t duration) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_ISEQUENCER_H_
