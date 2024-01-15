/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/participant_info.h
//! @brief Participant info.

#ifndef ROC_RTCP_PARTICIPANT_INFO_H_
#define ROC_RTCP_PARTICIPANT_INFO_H_

#include "roc_address/socket_addr.h"
#include "roc_packet/units.h"

namespace roc {
namespace rtcp {

//! Participant info.
//! @remarks
//!  Provides information about RTCP participant (sender/receiver).
struct ParticipantInfo {
    //! Participant CNAME.
    //! This string uniquely identifies each participant across all RTP sessions.
    //! It's used to associated related RTP sessions together.
    //! It's also used to distinguish SSRC collisions from network loops.
    const char* cname;

    //! Participant SSRC.
    //! This number uniquely identifies each participant within RTP session.
    //! If there is sending stream, its sender_source_id should be equal to it.
    //! If there are receivings streams, theirs receiver_source_id should be equal to it.
    packet::stream_source_t source_id;

    //! If set, reports are sent to this destination address.
    //! Used if report_back is false.
    //! Typically set on sender side.
    address::SocketAddr report_address;

    //! If set, reports are sent back to discovered senders.
    //! For every stream we send or receive via RTP, we remember address from which RTCP
    //! reports for that stream come. Then we send reports back to all such addresses.
    //! Typically set on receiver side.
    bool report_back;

    ParticipantInfo()
        : cname(NULL)
        , source_id(0)
        , report_address()
        , report_back(false) {
    }
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_PARTICIPANT_INFO_H_
