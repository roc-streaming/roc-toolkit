/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Stream Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/iparticipant.h
//! @brief RTCP participant.

#ifndef ROC_RTCP_IPARTICIPANT_H_
#define ROC_RTCP_IPARTICIPANT_H_

#include "roc_core/attributes.h"
#include "roc_packet/units.h"
#include "roc_rtcp/participant_info.h"
#include "roc_rtcp/reports.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtcp {

//! RTCP participant.
//!
//! Implemented by sender and receiver pipelines (see roc_pipeline module).
//!
//! Used by rtcp::Communicator to generate reports for local sending and/or receiving
//! streams, and to process reports from remote streams.
//!
//! One RTCP participant is usually associated with zero or one local sending stream and
//! one or a few (in case of multicast) remote sending streams.
//!
//! For the local sending stream, multiple remote receivers may exists. Communicator
//! will query one sending report from IParticipant for the sending stream, and
//! notify IParticipant with multiple receiving reports, one for every discovered
//! remote receiver.
//!
//! For each local receiving stream, there is corresponding remote sender.
//! Communicator will query receiving report from IParticipant for every local
//! receiving stream, as will notify IParticipant with corresponding sender
//! report for every local receiving stream.
//!
//! Single IParticipant instance usually corresponds to a single RTP session. However,
//! this is not a strict requirement: if configuration requires multiple related RTP
//! sessions to transfer single logical source, e.g. one RTP session for media packets
//! and another RTP session for FEC packets, then both RTP sessions will be associated
//! with a single IParticipant instance.
class IParticipant {
public:
    virtual ~IParticipant();

    //! Get local participant info.
    //! Invoked to know local CNAME, SSRC, etc.
    virtual ParticipantInfo participant_info() = 0;

    //! Change local SSRC to another randomly selected number.
    //! Invoked when SSRC collision is detected.
    virtual void change_source_id() = 0;

    //! Check whether pipeline has local sending stream.
    //! There can be only one local sending stream (or none).
    virtual bool has_send_stream() {
        return false;
    }

    //! Query sending report for local sending stream.
    //! Report will be used to generate RTCP packets for remote receivers.
    virtual SendReport query_send_stream(core::nanoseconds_t report_time) {
        return SendReport();
    }

    //! Notify local sending stream with receiver report.
    //! Report was gathered from RTCP packets from remote receiver.
    //! @p recv_source_id identifies remote receiver which sent report.
    //! In case of multicast sessions, one sending stream may have
    //! multiple receivers.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    notify_send_stream(packet::stream_source_t recv_source_id,
                       const RecvReport& recv_report) {
        return status::StatusOK;
    }

    //! Check how many local receiving streams are present.
    //! Multiple local receiving streams are allowed, each one corresponding to
    //! its own remote sender with unique sender SSRC.
    virtual size_t num_recv_streams() {
        return 0;
    }

    //! Query receiving reports from local receiving streams.
    //! Reports will be used to generate RTCP packets for remote senders.
    //! @p reports points to a buffer of @p n_reports size,
    //! where @p n_reports <= num_recv_streams().
    virtual void query_recv_streams(RecvReport* reports,
                                    size_t n_reports,
                                    core::nanoseconds_t report_time) {
    }

    //! Notify local receiving stream with sender report.
    //! Report was gathered from RTCP packets from remote sender.
    //! @p send_source_id identifies remote sender which sent report.
    //! If there are multiple receiving streams, each one will be notified
    //! with corresponding report.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    notify_recv_stream(packet::stream_source_t send_source_id,
                       const SendReport& send_report) {
        return status::StatusOK;
    }

    //! Terminate local receiving stream.
    //! Invoked when BYE packet is received from remote sender.
    //! @p send_source_id identifies remote sender which sent BYE.
    virtual void halt_recv_stream(packet::stream_source_t send_source_id) {
    }
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_IPARTICIPANT_H_
