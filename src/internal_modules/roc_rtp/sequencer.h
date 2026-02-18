/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/sequencer.h
//! @brief RTP packet sequencer.

#ifndef ROC_RTP_SEQUENCER_H_
#define ROC_RTP_SEQUENCER_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/isequencer.h"
#include "roc_rtp/identity.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtp {

//! RTP packet sequencer.
class Sequencer : public packet::ISequencer, public core::NonCopyable<> {
public:
    //! Initialize.
    Sequencer(Identity& identity, unsigned int payload_type);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Fill next packet.
    virtual void next(packet::Packet& packet,
                      core::nanoseconds_t capture_ts,
                      packet::stream_timestamp_t duration);

private:
    Identity& identity_;

    const unsigned int payload_type_;
    packet::seqnum_t seqnum_;
    packet::stream_timestamp_t stream_ts_;

    status::StatusCode init_status_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_SEQUENCER_H_
