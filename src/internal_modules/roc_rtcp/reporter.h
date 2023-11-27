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

#include "roc_core/array.h"
#include "roc_core/hashmap.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/ownership_policy.h"
#include "roc_core/ref_counted.h"
#include "roc_core/shared_ptr.h"
#include "roc_core/slab_pool.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"
#include "roc_rtcp/config.h"
#include "roc_rtcp/headers.h"
#include "roc_rtcp/istream_controller.h"
#include "roc_rtcp/reports.h"
#include "roc_rtcp/sdes.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtcp {

//! RTCP report processor and generator.
//!
//! Used by rtcp::Communicator to incrementally process and generate individual
//! blocks of compound RTCP packets. Collects data from RTCP traffic and local
//! stream controller (IStreamController).
//!
//! Features:
//!
//!  - Maintains hash table of all known sending and receiving streams.
//!    The table is populated from two sources: reports gathered via RTCP from
//!    remote peers and local reports gathered from stream controller.
//!
//!  - Provides methods to process report blocks from incoming RTCP packets.
//!    Incrementally fills stream table from provided report blocks.
//!    When RTCP packet is fully processed, notifies stream controller with
//!    the updated remote reports accumulated in stream table.
//!
//!  - Provides methods to generate report blocks for outgoing RTCP packets.
//!    Queries up-to-date local reports from stream controller into stream table.
//!    Incrementally fills report blocks from the stream table.
//!
//!  - Notifies stream controller when a stream is removed after receiving BYE
//!    message or due to inactivity timeout.
//!
//!  - Detects SSRC collisions and asks stream controller to switch SSRC.
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
    Reporter(const Config& config,
             IStreamController& stream_controller,
             core::IArena& arena);
    ~Reporter();

    //! Check if initialization succeeded.
    bool is_valid() const;

    //! Check if there is local sending stream.
    bool is_sending() const;

    //! Check if there are local receiving streams.
    bool is_receiving() const;

    //! Get number of active streams, for testing.
    size_t num_streams() const;

    //! @name Report processing
    //! @{

    //! Begin report processing.
    //! Invoked before process_xxx() functions.
    ROC_ATTR_NODISCARD status::StatusCode
    begin_processing(core::nanoseconds_t report_time);

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

    //! Generate SDES chunk with CNAME item.
    void generate_cname(SdesChunk& chunk, SdesItem& item);

    //! Generate SR header.
    void generate_sr(header::SenderReportPacket& sr);

    //! Generate RR header.
    void generate_rr(header::ReceiverReportPacket& rr);

    //! Get number of SR/RR reception blocks.
    size_t num_reception_blocks() const;

    //! Generate SR/RR reception block.
    //! @p index should be in range [0; num_reception_blocks()-1].
    void generate_reception_block(size_t index, header::ReceptionReportBlock& blk);

    //! Generate XR header.
    void generate_xr(header::XrPacket& xr);

    //! Get number of XR DLRR sub-blocks.
    size_t num_dlrr_subblocks() const;

    //! Generate XR DLRR sub-block.
    //! @p index should be in range [0; num_dlrr_subblocks()-1].
    void generate_dlrr_subblock(size_t index, header::XrDlrrSubblock& blk);

    //! Generate XR RRTR header (extended receiver report).
    void generate_rrtr_block(header::XrRrtrBlock& blk);

    //! Check if BYE message should be included.
    bool need_goodbye() const;

    //! Generate BYE message.
    void generate_goodbye(packet::stream_source_t& ssrc);

    //! End report generation.
    //! Invoked after generate_xxx() functions.
    ROC_ATTR_NODISCARD status::StatusCode end_generation();

    //! @}

private:
    enum { PreallocatedStreams = 8 };

    enum State {
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
                    core::HashmapNode,
                    core::ListNode {
        Stream(core::IPool& pool,
               packet::stream_source_t source_id,
               packet::stream_source_t report_time)
            : core::RefCounted<Stream, core::PoolAllocation>(pool)
            , source_id(source_id)
            , has_remote_recv_report(false)
            , has_remote_send_report(false)
            , last_update(report_time)
            , last_sr(0)
            , last_rr(0)
            , is_looped(false) {
            cname[0] = '\0';
        }

        // SSRC and CNAME of remote participant.
        packet::stream_source_t source_id;
        char cname[header::SdesItemHeader::MaxTextLen + 1];

        // Stream is sending to remote participant and we obtained
        // receiver report from it.
        bool has_remote_recv_report;
        RecvReport remote_recv_report;

        // Stream is receiving from remote participant and we obtained
        // sender report from it.
        bool has_remote_send_report;
        SendReport remote_send_report;

        // Stream is receiving from remote participant and this is our
        // receiver report to be delivered to remote side.
        // Such streams are added to local_recv_streams_ array.
        RecvReport local_recv_report;

        // Stream is sending to remote participant and this is our
        // sender report to be delivered to remote side.
        // Such streams are added to local_send_streams_ array.
        SendReport local_send_report;

        // Local timestamps.
        core::nanoseconds_t last_update;
        core::nanoseconds_t last_sr;
        core::nanoseconds_t last_rr;

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

    void notify_streams_();
    status::StatusCode query_send_streams_();
    status::StatusCode query_recv_streams_();

    void detect_timeouts_();
    void detect_collision_(packet::stream_source_t source_id);
    void resolve_collision_();

    void validate_send_report_(const SendReport& send_report);
    void validate_recv_report_(const RecvReport& recv_report);

    core::SharedPtr<Stream> find_stream_(packet::stream_source_t source_id,
                                         CreateMode mode);
    void remove_stream_(Stream& stream);
    void update_stream_(Stream& stream);

    IStreamController& stream_controller_;

    // Map of all streams, identified by SSRC
    core::SlabPool<Stream, PreallocatedStreams> stream_pool_;
    core::Hashmap<Stream, PreallocatedStreams> stream_map_;

    // List of all streams (from stream map) ordered by update time
    // Recently updated streams are moved to the front of the list
    core::List<Stream, core::NoOwnership> lru_streams_;

    // Pointers to local sending and receiving streams (from stream map),
    // as reported by stream controller
    core::Array<Stream*, PreallocatedStreams> local_send_streams_;
    core::Array<Stream*, PreallocatedStreams> local_recv_streams_;

    // Information obtained from stream controller
    char local_cname_[header::SdesItemHeader::MaxTextLen + 1];
    packet::stream_source_t local_source_id_;
    SendReport local_send_report_;

    // SSRC collision detection state
    bool collision_detected_;
    bool collision_reported_;

    // Report processing & generation state
    State report_state_;
    status::StatusCode report_error_;
    core::nanoseconds_t report_time_;

    // Inactivity timeout to remove dead streams
    const core::nanoseconds_t timeout_;

    bool valid_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_REPORTER_H_
