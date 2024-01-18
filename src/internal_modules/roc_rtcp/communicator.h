/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/communicator.h
//! @brief RTCP communicator.

#ifndef ROC_RTCP_COMMUNICATOR_H_
#define ROC_RTCP_COMMUNICATOR_H_

#include "roc_core/buffer_factory.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/rate_limiter.h"
#include "roc_core/stddefs.h"
#include "roc_core/time.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_factory.h"
#include "roc_rtcp/builder.h"
#include "roc_rtcp/config.h"
#include "roc_rtcp/iparticipant.h"
#include "roc_rtcp/reporter.h"
#include "roc_rtcp/traverser.h"

namespace roc {
namespace rtcp {

//! RTCP communicator.
//!
//! Implements bidirectional exchange of RTCP packets with other participants
//! of a single RTP session.
//!
//! Holds a reference to IParticipant interface, which is implemented by
//! sender or receiver pipeline.
//!
//! Features:
//!
//!  - processes received RTCP packets, extract reports from packets,
//!    and notifies IParticipant with reports from remote side
//!
//!  - queries IParticipant with up-to-date reports from local
//!    side, and generates RTCP packets to be sent to remote side
//!
//! For more details about streams and reports, @see IParticipant.
//!
//! This is top-level class of roc_rtcp module, glueing together other components:
//!   - rtcp::Traverser, to iterate through blocks of compound RTCP packets
//!   - rtcp::Builder, to construct compound RTCP packets
//!   - rtcp::Reporter, to maintain hash table of active streams, process and generate
//!     individual blocks of compound packets, and interact with IParticipant
class Communicator : public core::NonCopyable<> {
public:
    //! Initialize.
    Communicator(const Config& config,
                 IParticipant& participant,
                 packet::IWriter& packet_writer,
                 packet::IComposer& packet_composer,
                 packet::PacketFactory& packet_factory,
                 core::BufferFactory<uint8_t>& buffer_factory,
                 core::IArena& arena);

    //! Check if initialization succeeded.
    bool is_valid() const;

    //! Get number of tracked streams, for testing.
    size_t num_streams() const;

    //! Get number of tracked destination addresses, for testing.
    size_t num_destinations() const;

    //! Parse and process incoming packet.
    //! Invokes IParticipant methods during processing.
    ROC_ATTR_NODISCARD status::StatusCode
    process_packet(const packet::PacketPtr& packet, core::nanoseconds_t current_time);

    //! When we should generate packets next time.
    //! Returns absolute time.
    //! @p current_time is current time in nanoseconds since Unix epoch.
    core::nanoseconds_t generation_deadline(core::nanoseconds_t current_time);

    //! Generate and send report packet(s).
    //! Should be called accroding to generation_deadline().
    //! @p current_time is current time in nanoseconds since Unix epoch.
    //! Invokes IParticipant methods during generation.
    ROC_ATTR_NODISCARD status::StatusCode
    generate_reports(core::nanoseconds_t current_time);

    //! Generate and send goodbye packet(s).
    //! Should be called before termination sender session.
    //! @p current_time is current time in nanoseconds since Unix epoch.
    //! Invokes IParticipant methods during generation.
    ROC_ATTR_NODISCARD status::StatusCode
    generate_goodbye(core::nanoseconds_t current_time);

private:
    enum PacketType { PacketType_Reports, PacketType_Goodbye };

    void process_all_descriptions_(const Traverser& traverser);
    void process_all_reports_(const Traverser& traverser);
    void process_all_goodbyes_(const Traverser& traverser);

    void process_description_(const SdesTraverser& sdes);
    void process_goodbye_(const ByeTraverser& bye);
    void process_sender_report_(const header::SenderReportPacket& sr);
    void process_receiver_report_(const header::ReceiverReportPacket& rr);
    void process_extended_report_(const XrTraverser& xr);

    status::StatusCode generate_packets_(core::nanoseconds_t current_time,
                                         PacketType packet_type);

    status::StatusCode begin_packet_generation_(core::nanoseconds_t current_time);
    status::StatusCode end_packet_generation_();
    bool continue_packet_generation_();
    status::StatusCode write_generated_packet_(const packet::PacketPtr& packet);

    status::StatusCode generate_packet_(PacketType packet_type,
                                        packet::PacketPtr& packet);

    status::StatusCode generate_packet_payload_(PacketType packet_type,
                                                core::Slice<uint8_t>& packet_payload);

    void generate_reports_payload_(Builder& bld);
    void generate_goodbye_payload_(Builder& bld);

    void generate_standard_report_(Builder& bld);
    void generate_extended_report_(Builder& bld);
    void generate_empty_report_(Builder& bld);
    void generate_description_(Builder& bld);
    void generate_goodbye_(Builder& bld);

    void record_error_();

    packet::PacketFactory& packet_factory_;
    core::BufferFactory<uint8_t>& buffer_factory_;

    packet::IWriter& packet_writer_;
    packet::IComposer& packet_composer_;

    const Config config_;
    Reporter reporter_;

    // When generation_deadline() should be called next time.
    core::nanoseconds_t next_deadline_;

    // Maximum number of destination addresses, and index of current destination
    // address for which we're generating report.
    size_t max_dest_addrs_;
    size_t cur_dest_addr_;

    // Maximum number of SR/RR/XR blocks per packet, and number of current block
    // in packet. If maximum is exceeded, report is split into multiple packets.
    size_t max_pkt_blocks_;
    size_t cur_pkt_block_;

    size_t srrr_index_; // Current SR/RR block.
    size_t srrr_max_;   // Total count of SR/RR blocks in report.
    size_t dlrr_index_; // Current DLRR block.
    size_t dlrr_max_;   // Total count of DLRR blocks in report.

    // Dropped malformed incoming packets.
    size_t error_count_;
    core::RateLimiter error_limiter_;

    bool valid_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_COMMUNICATOR_H_
