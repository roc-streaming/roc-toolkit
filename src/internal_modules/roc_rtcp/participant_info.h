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

//! Participant report generation mode.
enum ParticipantReportMode {
    //! Reports are sent to a single static destination address,
    //! set via report_address field of ParticipantInfo struct.
    //! This mode is typically used on sender side.
    Report_ToAddress,

    //! Reports are sent back to dynamically discovered participant.
    //! In this mode, for every stream we send or receive via RTP, we remember
    //! address from which RTCP reports for that stream come. Then we send
    //! reports back to all such addresses.
    //! This mode is typically used on receiver side.
    Report_Back,
};

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

    //! Participant report mode.
    //! Deterimeds where to send generated reports.
    ParticipantReportMode report_mode;

    //! Participant destination report address.
    //! Used if report_mode is set to Report_ToAddress.
    address::SocketAddr report_address;

    ParticipantInfo()
        : cname(NULL)
        , source_id(0)
        , report_mode(Report_ToAddress)
        , report_address() {
    }
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_PARTICIPANT_INFO_H_
