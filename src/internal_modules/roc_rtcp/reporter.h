/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/reporter.h
//! @brief RTCP reporter.

#ifndef ROC_RTCP_REPORTER_H_
#define ROC_RTCP_REPORTER_H_

#include "roc_address/socket_addr.h"
#include "roc_core/array.h"
#include "roc_core/hashmap.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/slab_pool.h"
#include "roc_core/time.h"
#include "roc_packet/ntp.h"
#include "roc_packet/units.h"
#include "roc_rtcp/cname.h"
#include "roc_rtcp/config.h"
#include "roc_rtcp/headers.h"
#include "roc_rtcp/iparticipant.h"
#include "roc_rtcp/loss_estimator.h"
#include "roc_rtcp/packet_counter.h"
#include "roc_rtcp/reports.h"
#include "roc_rtcp/rtt_estimator.h"
#include "roc_rtcp/sdes.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtcp {

//! RTCP report processor and generator.
//!
//! Used by rtcp::Communicator to incrementally process and generate individual
//! blocks of compound RTCP packets. Collects data from RTCP traffic and local
//! pipeline (IParticipant).
//!
//! Features:
//!
//!  - Maintains hash table of all known sending and receiving streams.
//!    The table is populated from two sources: reports gathered via RTCP from
//!    remote peers and local reports gathered from IParticipant.
//!
//!  - Maintains hash table of all destination addresses where to send reports,
//!    and an index to quickly find which streams are associated with each address.
//!
//!  - Provides methods to process report blocks from incoming RTCP packets.
//!    Incrementally fills internal tables from provided report blocks.
//!    When RTCP packet is fully processed, notifies IParticipant with
//!    the updated remote reports accumulated in internal tables.
//!
//!  - Provides methods to generate report blocks for outgoing RTCP packets.
//!    Queries up-to-date local reports from IParticipant into internal tables.
//!    Incrementally fills report blocks from the internal tables.
//!
//!  - Notifies IParticipant when a stream is removed after receiving BYE
//!    message or due to inactivity timeout.
//!
//!  - Detects SSRC collisions and asks IParticipant to switch SSRC.
//!    Sends BYE message for old SSRC.
//!
//! Workflow of incoming packets processing:
//!
//! @code
//!   reporter.begin_processing()
//!   reporter.process_sr(...)
//!   reporter.process_reception_block(...)
//!   ...
//!   reporter.end_processing()
//! @endcode
//!
//! Workflow of outgoing packet generation:
//!
//! @code
//!   reporter.begin_generation();
//!   reporter.generate_sr(...)
//!   reporter.generate_reception_block(...)
//!   ...
//!   reporter.end_generation()
//! @endcode
class Reporter : public core::NonCopyable<> {
public:
    //! Initialize.
    Reporter(const Config& config, IParticipant& participant, core::IArena& arena);
    ~Reporter();

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Check if there is local sending stream.
    bool is_sending() const;

    //! Check if there are local receiving streams.
    bool is_receiving() const;

    //! Get number of tracked destination addresses, for testing.
    size_t total_destinations() const;

    //! Get number of tracked streams, for testing.
    size_t total_streams() const;

    //! @name Report processing
    //! @{

    //! Begin report processing.
    //! Invoked before process_xxx() functions.
    ROC_ATTR_NODISCARD status::StatusCode
    begin_processing(const address::SocketAddr& report_addr,
                     core::nanoseconds_t report_time);

    //! Process SDES CNAME.
    void process_cname(const SdesChunk& chunk, const SdesItem& item);

    //! Process SR header.
    void process_sr(const header::SenderReportPacket& sr);

    //! Process SR/RR reception block.
    void process_reception_block(packet::stream_source_t ssrc,
                                 const header::ReceptionReportBlock& blk);

    //! Process XR DLRR sub-block (extended sender report).
    void process_dlrr_subblock(const header::XrPacket& xr,
                               const header::XrDlrrSubblock& blk);

    //! Process XR RRTR block (extended receiver report).
    void process_rrtr_block(const header::XrPacket& xr, const header::XrRrtrBlock& blk);

    //! Process XR Measurement Info block (extended receiver report).
    void process_measurement_info_block(const header::XrPacket& xr,
                                        const header::XrMeasurementInfoBlock& blk);

    //! Process XR Delay Metrics block (extended receiver report).
    void process_delay_metrics_block(const header::XrPacket& xr,
                                     const header::XrDelayMetricsBlock& blk);

    //! Process XR Queue Metrics block (extended receiver report).
    void process_queue_metrics_block(const header::XrPacket& xr,
                                     const header::XrQueueMetricsBlock& blk);

    //! Process BYE message.
    void process_goodbye(packet::stream_source_t ssrc);

    //! End report processing.
    //! Invoked after process_xxx() functions.
    ROC_ATTR_NODISCARD status::StatusCode end_processing();

    //! @}

    //! @name Report generation
    //! @{

    //! Begin report generation.
    //! Invoked before genrate_xxx() functions.
    ROC_ATTR_NODISCARD status::StatusCode
    begin_generation(core::nanoseconds_t report_time);

    //! Get number of destination addresses to which to send reports.
    size_t num_dest_addresses() const;

    //! Get number of sending streams to be reported.
    //! @p addr_index should be in range [0; num_dest_addresses()-1].
    size_t num_sending_streams(size_t addr_index) const;

    //! Get number of receiving streams to be reported.
    //! @p addr_index should be in range [0; num_dest_addresses()-1].
    size_t num_receiving_streams(size_t addr_index) const;

    //! Generate destination address.
    //! @p addr_index should be in range [0; num_dest_addresses()-1].
    void generate_dest_address(size_t addr_index, address::SocketAddr& addr);

    //! Generate SDES chunk with CNAME item.
    void generate_cname(SdesChunk& chunk, SdesItem& item);

    //! Generate SR header.
    void generate_sr(header::SenderReportPacket& sr);

    //! Generate RR header.
    void generate_rr(header::ReceiverReportPacket& rr);

    //! Generate SR/RR reception block.
    //! @p addr_index should be in range [0; num_dest_addresses()-1].
    //! @p stream_index should be in range [0; num_receiving_streams()-1].
    void generate_reception_block(size_t addr_index,
                                  size_t stream_index,
                                  header::ReceptionReportBlock& blk);

    //! Generate XR header.
    void generate_xr(header::XrPacket& xr);

    //! Generate XR DLRR sub-block (extended sender report).
    //! @p addr_index should be in range [0; num_dest_addresses()-1].
    //! @p stream_index should be in range [0; num_sending_streams()-1].
    void generate_dlrr_subblock(size_t addr_index,
                                size_t stream_index,
                                header::XrDlrrSubblock& blk);

    //! Generate XR RRTR header (extended receiver report).
    void generate_rrtr_block(header::XrRrtrBlock& blk);

    //! Generate XR Measurement Info block (extended receiver report).
    //! @p addr_index should be in range [0; num_dest_addresses()-1].
    //! @p stream_index should be in range [0; num_receiving_streams()-1].
    void generate_measurement_info_block(size_t addr_index,
                                         size_t stream_index,
                                         header::XrMeasurementInfoBlock& blk);

    //! Generate XR Delay Metrics block (extended receiver report).
    //! @p addr_index should be in range [0; num_dest_addresses()-1].
    //! @p stream_index should be in range [0; num_receiving_streams()-1].
    void generate_delay_metrics_block(size_t addr_index,
                                      size_t stream_index,
                                      header::XrDelayMetricsBlock& blk);

    //! Generate XR Queue Metrics block (extended receiver report).
    //! @p addr_index should be in range [0; num_dest_addresses()-1].
    //! @p stream_index should be in range [0; num_receiving_streams()-1].
    void generate_queue_metrics_block(size_t addr_index,
                                      size_t stream_index,
                                      header::XrQueueMetricsBlock& blk);

    //! Check if BYE message should be included.
    bool need_goodbye() const;

    //! Generate BYE message.
    void generate_goodbye(packet::stream_source_t& ssrc);

    //! End report generation.
    //! Invoked after generate_xxx() functions.
    ROC_ATTR_NODISCARD status::StatusCode end_generation();

    //! @}

private:
    enum { PreallocatedStreams = 8, PreallocatedAddresses = 4 };

    enum ReportState {
        State_Idle,       // Default state
        State_Processing, // Between begin_processing() and end_processing()
        State_Generating, // Between begin_generation() and end_generation()
    };

    enum CreateMode {
        AutoCreate,   // Automatically create stream if not found
        NoAutoCreate, // Return NULL if not found
    };

    // Represents state of one local sending and/or receiving stream.
    // One stream object is created for every discovered remote participant
    // that receives from us and/or sends to us.
    // Stream is uniquely identified by SSRC of remote participant.
    struct Stream : core::RefCounted<Stream, core::PoolAllocation>,
                    core::HashmapNode<>,
                    core::ListNode<> {
        Stream(core::IPool& pool,
               packet::stream_source_t source_id,
               core::nanoseconds_t report_time,
               const RttConfig& rtt_config)
            : core::RefCounted<Stream, core::PoolAllocation>(pool)
            , source_id(source_id)
            , has_remote_recv_report(false)
            , remote_recv_rtt(rtt_config)
            , has_remote_send_report(false)
            , remote_send_rtt(rtt_config)
            , local_recv_report(NULL)
            , last_update(report_time)
            , last_local_sr(0)
            , last_remote_rr(0)
            , last_remote_rr_ntp(0)
            , last_remote_dlsr(0)
            , last_local_rr(0)
            , last_remote_sr(0)
            , last_remote_sr_ntp(0)
            , last_remote_dlrr(0)
            , is_looped(false) {
            cname[0] = '\0';
        }

        // SSRC and CNAME of remote participant.
        packet::stream_source_t source_id;
        char cname[MaxCnameLen + 1];

        // Stream is sending to remote participant and we obtained
        // receiver report from it.
        bool has_remote_recv_report;
        RecvReport remote_recv_report;
        RttEstimator remote_recv_rtt;
        PacketCounter remote_recv_packet_count;

        // Stream is receiving from remote participant and we obtained
        // sender report from it.
        bool has_remote_send_report;
        SendReport remote_send_report;
        RttEstimator remote_send_rtt;
        PacketCounter remote_send_packet_count;
        PacketCounter remote_send_byte_count;

        // Stream is receiving from remote participant and this is our
        // receiver report to be delivered to remote side.
        // Points to an element of local_recv_reports_ array. Whenever
        // array is resized, rebuild_index_() updates the pointers.
        RecvReport* local_recv_report;
        LossEstimator local_recv_loss;

        // Remote address from where reports are coming.
        address::SocketAddr remote_address;

        // Whenever stream is updated, this timestamp changes and stream
        // is moved to the front of stream_lru_ list.
        core::nanoseconds_t last_update;

        // When we sent last SR for which we received DLSR (local clock).
        core::nanoseconds_t last_local_sr;
        // When we received last RR (local clock).
        core::nanoseconds_t last_remote_rr;
        // NTP timestamp from last RR (as it was in packet, remote clock).
        packet::ntp_timestamp_t last_remote_rr_ntp;
        // DLSR received with last RR (delta, remote clock).
        core::nanoseconds_t last_remote_dlsr;

        // When we sent last RR for which we received DLRR (local clock).
        core::nanoseconds_t last_local_rr;
        // When we received last SR (local clock).
        core::nanoseconds_t last_remote_sr;
        // NTP timestamp from last SR (as it was in packet, remote clock).
        packet::ntp_timestamp_t last_remote_sr_ntp;
        // DLRR received with last SR (delta, remote clock).
        core::nanoseconds_t last_remote_dlrr;

        // Set when we detect network loop.
        bool is_looped;

        packet::stream_source_t key() const {
            return source_id;
        }

        static core::hashsum_t key_hash(packet::stream_source_t id) {
            return core::hashsum_int(id);
        }

        static bool key_equal(packet::stream_source_t id1, packet::stream_source_t id2) {
            return id1 == id2;
        }
    };

    // Represents one destination address.
    // If we're sending all reports to a single preconfigured address, there will be
    // only one instance. Otherwise there will be an instance for every unique address.
    struct Address : core::RefCounted<Address, core::PoolAllocation>,
                     core::HashmapNode<>,
                     core::ListNode<> {
        Address(core::IPool& pool,
                core::IArena& arena,
                const address::SocketAddr& remote_address,
                core::nanoseconds_t report_time)
            : core::RefCounted<Address, core::PoolAllocation>(pool)
            , remote_address(remote_address)
            , send_stream_index(arena)
            , recv_stream_index(arena)
            , last_rebuild(report_time) {
        }

        // Destination address where to send reports.
        address::SocketAddr remote_address;

        // Pointers to local sending and receiving streams from stream map
        // associated with given address.
        core::Array<Stream*, PreallocatedStreams> send_stream_index;
        core::Array<Stream*, PreallocatedStreams> recv_stream_index;

        // Whenever address is rebuilt, this timestamp changes and address
        // is moved to the front of address_lru_ list.
        core::nanoseconds_t last_rebuild;

        const address::SocketAddr& key() const {
            return remote_address;
        }

        static core::hashsum_t key_hash(const address::SocketAddr& addr) {
            return core::hashsum_mem(addr.saddr(), (size_t)addr.slen());
        }

        static bool key_equal(const address::SocketAddr& addr1,
                              const address::SocketAddr& addr2) {
            return addr1 == addr2;
        }
    };

    status::StatusCode notify_streams_();
    status::StatusCode refresh_streams_();
    status::StatusCode query_streams_();
    status::StatusCode rebuild_index_();

    void detect_timeouts_();
    void detect_collision_(packet::stream_source_t source_id);
    void resolve_collision_();

    void validate_send_report_(const SendReport& send_report);
    void validate_recv_report_(const RecvReport& recv_report);

    core::SharedPtr<Stream> find_stream_(packet::stream_source_t source_id,
                                         CreateMode mode);
    void remove_stream_(Stream& stream);
    void update_stream_(Stream& stream);

    core::SharedPtr<Address> find_address_(const address::SocketAddr& remote_address,
                                           CreateMode mode);
    void remove_address_(Address& address);
    void rebuild_address_(Address& address);

    core::IArena& arena_;

    // Interface implemented by local sender/receiver pipeline.
    IParticipant& participant_;

    // Defines whether participant uses a single static destination address
    // for all all reports, or otherwise sends individual reports to dynamically
    // discovered remote addresses.
    ParticipantReportMode participant_report_mode_;
    address::SocketAddr participant_report_addr_;

    // Information obtained from IParticipant.
    char local_cname_[MaxCnameLen + 1];
    packet::stream_source_t local_source_id_;
    bool has_local_send_report_;
    SendReport local_send_report_;
    core::Array<RecvReport, PreallocatedStreams> local_recv_reports_;

    // Map of all streams, identified by SSRC.
    core::SlabPool<Stream, PreallocatedStreams> stream_pool_;
    core::Hashmap<Stream, PreallocatedStreams> stream_map_;

    // List of all streams (from stream map) ordered by update time.
    // Recently updated streams are moved to the front of the list.
    // This list always contains all existing streams.
    core::List<Stream, core::NoOwnership> stream_lru_;

    // Map of all destination addresses.
    // In Report_ToAddress mode, there will be only one address.
    // In Report_Back mode, addresses will be allocated as we discover
    // new remote participants.
    core::SlabPool<Address, PreallocatedAddresses> address_pool_;
    core::Hashmap<Address, PreallocatedAddresses> address_map_;

    // List of all addresses (from address map) ordered by rebuild time.
    // Recently rebuilt addresses are moved to the front of the list.
    // This list always contains all existing addresses.
    core::List<Address, core::NoOwnership> address_lru_;

    // Pointers to addresses (from address map), which in turn hold
    // pointers to streams (from stream map), for fast access by index
    // during report generation.
    core::Array<Address*, PreallocatedAddresses> address_index_;
    // If true, the index should be rebuilt before next generation.
    bool need_rebuild_index_;

    // When we sent most recent SR (local clock).
    core::nanoseconds_t current_sr_;
    // When we sent most recent RR (local clock).
    core::nanoseconds_t current_rr_;

    // SSRC collision detection state.
    bool collision_detected_;
    bool collision_reported_;

    // Report processing & generation state.
    ReportState report_state_;
    status::StatusCode report_error_;
    address::SocketAddr report_addr_;
    core::nanoseconds_t report_time_;

    // Configuration.
    const Config config_;
    const core::nanoseconds_t max_delay_;

    status::StatusCode init_status_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_REPORTER_H_
