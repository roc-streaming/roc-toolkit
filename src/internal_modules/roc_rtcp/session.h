/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/session.h
//! @brief RTCP session.

#ifndef ROC_RTCP_SESSION_H_
#define ROC_RTCP_SESSION_H_

#include "roc_core/buffer_factory.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtcp/builder.h"
#include "roc_rtcp/ireceiver_hooks.h"
#include "roc_rtcp/isender_hooks.h"
#include "roc_rtcp/traverser.h"

namespace roc {
namespace rtcp {

//! RTCP session.
//! Processes incoming RTCP packets and generates outgoing RTCP packets.
class Session {
public:
    //! Initialize.
    Session(IReceiverHooks* recv_hooks,
            ISenderHooks* send_hooks,
            packet::IWriter* packet_writer,
            packet::IComposer& packet_composer,
            packet::PacketFactory& packet_factory,
            core::BufferFactory<uint8_t>& buffer_factory);

    //! Check if initialization succeeded.
    bool is_valid() const;

    //! Parse and process incoming packet.
    //! Invokes session hooks methods during processing.
    void process_packet(const packet::PacketPtr& packet);

    //! When we should generate packets next time.
    //! Returns absolute time.
    core::nanoseconds_t generation_deadline();

    //! Generate and send packet(s).
    //! Should be called accroding to generation_deadline().
    void generate_packets();

private:
    void parse_events_(const Traverser& traverser);
    void parse_reports_(const Traverser& traverser);

    void parse_session_description_(const SdesTraverser& sdes);
    void parse_goodbye_(const ByeTraverser& bye);
    void parse_sender_report_(const header::SenderReportPacket& sr);
    void parse_receiver_report_(const header::ReceiverReportPacket& rr);
    void parse_reception_block_(const header::ReceptionReportBlock& blk);

    packet::PacketPtr generate_packet_();

    bool build_packet_(core::Slice<uint8_t>& data);
    void build_sender_report_(Builder& bld, packet::ntp_timestamp_t report_time);
    void build_receiver_report_(Builder& bld, packet::ntp_timestamp_t report_time);
    header::ReceptionReportBlock build_reception_block_(const ReceptionMetrics& metrics);
    void build_session_description_(Builder& bld);
    void build_source_description_(Builder& bld, packet::source_t ssrc);

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& buffer_factory_;

    packet::IWriter* packet_writer_;
    packet::IComposer& packet_composer_;

    IReceiverHooks* recv_hooks_;
    ISenderHooks* send_hooks_;

    core::nanoseconds_t next_deadline_;

    packet::source_t ssrc_;
    char cname_[header::SdesItemHeader::MaxTextLen + 1];

    bool valid_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_SESSION_H_
