/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/communicator.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/time.h"
#include "roc_packet/ntp.h"
#include "roc_packet/units.h"
#include "roc_rtcp/headers.h"
#include "roc_status/code_to_str.h"
#include "roc_status/status_code.h"

namespace roc {
namespace rtcp {

namespace {

const core::nanoseconds_t LogInterval = core::Second * 30;

} // namespace

Communicator::Communicator(const Config& config,
                           IParticipant& participant,
                           packet::IWriter& packet_writer,
                           packet::IComposer& packet_composer,
                           packet::PacketFactory& packet_factory,
                           core::IArena& arena)
    : packet_factory_(packet_factory)
    , packet_writer_(packet_writer)
    , packet_composer_(packet_composer)
    , config_(config)
    , reporter_(config, participant, arena)
    , next_deadline_(0)
    , dest_addr_count_(0)
    , dest_addr_index_(0)
    , send_stream_count_(0)
    , send_stream_index_(0)
    , recv_stream_count_(0)
    , recv_stream_index_(0)
    , max_pkt_streams_(header::MaxPacketBlocks)
    , cur_pkt_send_stream_(0)
    , cur_pkt_recv_stream_(0)
    , error_count_(0)
    , processed_packet_count_(0)
    , generated_packet_count_(0)
    , log_limiter_(LogInterval)
    , init_status_(status::NoStatus) {
    if ((init_status_ = reporter_.init_status()) != status::StatusOK) {
        return;
    }
    init_status_ = status::StatusOK;
}

status::StatusCode Communicator::init_status() const {
    return init_status_;
}

size_t Communicator::total_destinations() const {
    return reporter_.total_destinations();
}

size_t Communicator::total_streams() const {
    return reporter_.total_streams();
}

status::StatusCode Communicator::process_packet(const packet::PacketPtr& packet,
                                                core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if_msg(!packet, "rtcp communicator: null packet");
    roc_panic_if_msg(!packet->udp(), "rtcp communicator: non-udp packet");
    roc_panic_if_msg(!packet->rtcp(), "rtcp communicator: non-rtcp packet");
    roc_panic_if_msg(current_time <= 0, "rtcp communicator: invalid timestamp");

    roc_log(LogTrace, "rtcp communicator: processing incoming packet");

    processed_packet_count_++;

    Traverser traverser(packet->rtcp()->payload);
    if (!traverser.parse()) {
        roc_log(LogTrace, "rtcp communicator: error when parsing compound packet");
        error_count_++;
        return status::StatusOK;
    }

    status::StatusCode status =
        reporter_.begin_processing(packet->udp()->src_addr, current_time);
    roc_log(LogTrace, "rtcp communicator: begin_processing(): status=%s",
            status::code_to_str(status));

    if (status != status::StatusOK) {
        roc_log(LogDebug, "rtcp communicator: processing failed: status=%s",
                status::code_to_str(status));
        return status;
    }

    // First parse SDES packets to create/recreate/update streams.
    process_all_descriptions_(traverser);
    // Then parse SR, RR, and XR to create/update streams.
    process_all_reports_(traverser);
    // Then parse BYE packets to terminate streams.
    process_all_goodbyes_(traverser);

    status = reporter_.end_processing();
    roc_log(LogTrace, "rtcp communicator: end_processing(): status=%s",
            status::code_to_str(status));

    if (status != status::StatusOK) {
        roc_log(LogDebug, "rtcp communicator: processing failed: status=%s",
                status::code_to_str(status));
        return status;
    }

    return status::StatusOK;
}

void Communicator::process_all_descriptions_(const Traverser& traverser) {
    Traverser::Iterator iter = traverser.iter();
    Traverser::Iterator::State state;

    while ((state = iter.next()) != Traverser::Iterator::END) {
        switch (state) {
        case Traverser::Iterator::SDES: {
            SdesTraverser sdes = iter.get_sdes();
            if (!sdes.parse()) {
                roc_log(LogTrace, "rtcp communicator: error when parsing SDES packet");
                error_count_++;
                break;
            }
            process_description_(sdes);
        } break;

        default:
            break;
        }
    }

    if (iter.error()) {
        roc_log(LogTrace, "rtcp communicator: error when traversing compound packet");
        error_count_++;
    }
}

void Communicator::process_all_reports_(const Traverser& traverser) {
    Traverser::Iterator iter = traverser.iter();
    Traverser::Iterator::State state;

    while ((state = iter.next()) != Traverser::Iterator::END) {
        switch (state) {
        case Traverser::Iterator::SR: {
            process_sender_report_(iter.get_sr());
        } break;

        case Traverser::Iterator::RR: {
            process_receiver_report_(iter.get_rr());
        } break;

        case Traverser::Iterator::XR: {
            XrTraverser xr = iter.get_xr();
            if (!xr.parse()) {
                roc_log(LogTrace, "rtcp communicator: error when parsing XR packet");
                error_count_++;
                break;
            }
            process_extended_report_(xr);
        } break;

        default:
            break;
        }
    }

    if (iter.error()) {
        roc_log(LogTrace, "rtcp communicator: error when traversing compound packet");
        error_count_++;
    }
}

void Communicator::process_all_goodbyes_(const Traverser& traverser) {
    Traverser::Iterator iter = traverser.iter();
    Traverser::Iterator::State state;

    while ((state = iter.next()) != Traverser::Iterator::END) {
        switch (state) {
        case Traverser::Iterator::BYE: {
            ByeTraverser bye = iter.get_bye();
            if (!bye.parse()) {
                roc_log(LogTrace, "rtcp communicator: error when parsing BYE packet");
                error_count_++;
                break;
            }
            process_goodbye_(bye);
        } break;

        default:
            break;
        }
    }

    if (iter.error()) {
        roc_log(LogTrace, "rtcp communicator: error when traversing compound packet");
        error_count_++;
    }
}

void Communicator::process_description_(const SdesTraverser& sdes) {
    SdesTraverser::Iterator iter = sdes.iter();
    SdesTraverser::Iterator::State state;

    SdesChunk sdes_chunk;

    while ((state = iter.next()) != SdesTraverser::Iterator::END) {
        switch (state) {
        case SdesTraverser::Iterator::CHUNK: {
            sdes_chunk = iter.get_chunk();
        } break;

        case SdesTraverser::Iterator::ITEM: {
            const SdesItem sdes_item = iter.get_item();
            if (sdes_item.type != header::SDES_CNAME) {
                continue;
            }
            if (sdes_item.text[0] == '\0') {
                continue;
            }
            reporter_.process_cname(sdes_chunk, sdes_item);
        } break;

        default:
            break;
        }
    }

    if (iter.error()) {
        roc_log(LogTrace, "rtcp communicator: error when traversing SDES packet");
        error_count_++;
    }
}

void Communicator::process_goodbye_(const ByeTraverser& bye) {
    ByeTraverser::Iterator iter = bye.iter();
    ByeTraverser::Iterator::Iterator::State state;

    while ((state = iter.next()) != ByeTraverser::Iterator::END) {
        switch (state) {
        case ByeTraverser::Iterator::SSRC:
            reporter_.process_goodbye(iter.get_ssrc());
            break;

        default:
            break;
        }
    }

    if (iter.error()) {
        roc_log(LogTrace, "rtcp communicator: error when traversing BYE packet");
        error_count_++;
    }
}

void Communicator::process_sender_report_(const header::SenderReportPacket& sr) {
    // SR header contains sending report.
    reporter_.process_sr(sr);

    // Optional reception blocks after SR header are used when remote sender also acts
    // as receiver. In this case reception report blocks provide receiver reports.
    for (size_t n = 0; n < sr.num_blocks(); n++) {
        reporter_.process_reception_block(sr.ssrc(), sr.get_block(n));
    }
}

void Communicator::process_receiver_report_(const header::ReceiverReportPacket& rr) {
    // RR contains only reception blocks with receiver reports.
    for (size_t n = 0; n < rr.num_blocks(); n++) {
        reporter_.process_reception_block(rr.ssrc(), rr.get_block(n));
    }
}

void Communicator::process_extended_report_(const XrTraverser& xr) {
    XrTraverser::Iterator iter = xr.iter();
    XrTraverser::Iterator::Iterator::State state;

    while ((state = iter.next()) != XrTraverser::Iterator::END) {
        switch (state) {
        case XrTraverser::Iterator::DLRR_BLOCK: {
            // DLRR is extended sender report.
            const header::XrDlrrBlock& dlrr = iter.get_dlrr();

            for (size_t n = 0; n < dlrr.num_subblocks(); n++) {
                reporter_.process_dlrr_subblock(xr.packet(), dlrr.get_subblock(n));
            }
        } break;

        case XrTraverser::Iterator::RRTR_BLOCK: {
            // RRTR is extended receiver report.
            reporter_.process_rrtr_block(xr.packet(), iter.get_rrtr());
        } break;

        case XrTraverser::Iterator::MEASUREMENT_INFO_BLOCK: {
            // Measurement Info is extended receiver report.
            reporter_.process_measurement_info_block(xr.packet(),
                                                     iter.get_measurement_info());
        } break;

        case XrTraverser::Iterator::DELAY_METRICS_BLOCK: {
            // Delay Metrics is extended receiver report.
            reporter_.process_delay_metrics_block(xr.packet(), iter.get_delay_metrics());
        } break;

        case XrTraverser::Iterator::QUEUE_METRICS_BLOCK: {
            // Queue Metrics is extended receiver report.
            reporter_.process_queue_metrics_block(xr.packet(), iter.get_queue_metrics());
        } break;

        default:
            break;
        }
    }

    if (iter.error()) {
        roc_log(LogTrace, "rtcp communicator: error when traversing XR packet");
        error_count_++;
    }
}

core::nanoseconds_t Communicator::generation_deadline(core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if_msg(current_time <= 0,
                     "rtcp communicator: invalid timestamp:"
                     " expected positive value, got %lld",
                     (long long)current_time);

    if (next_deadline_ == 0) {
        // Until generate_packets() is called first time, report that
        // we're ready immediately.
        next_deadline_ = current_time;
    }

    return next_deadline_;
}

status::StatusCode Communicator::generate_reports(core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if_msg(current_time <= 0,
                     "rtcp communicator: invalid timestamp:"
                     " expected positive value, got %lld",
                     (long long)current_time);

    if (next_deadline_ == 0) {
        next_deadline_ = current_time;
    }

    if (next_deadline_ > current_time) {
        return status::StatusOK;
    }

    // TODO(gh-674): use IntervalComputer
    next_deadline_ = current_time + config_.report_interval
        - ((current_time - next_deadline_) % config_.report_interval);

    roc_log(LogTrace, "rtcp communicator: generating report packets");

    const status::StatusCode status = generate_packets_(current_time, PacketType_Reports);
    if (status != status::StatusOK) {
        roc_log(LogDebug, "rtcp communicator: generation failed: status=%s",
                status::code_to_str(status));
    }

    return status;
}

status::StatusCode Communicator::generate_goodbye(core::nanoseconds_t current_time) {
    roc_panic_if(init_status_ != status::StatusOK);

    roc_panic_if_msg(current_time <= 0, "rtcp communicator: invalid timestamp");

    roc_log(LogTrace, "rtcp communicator: generating goodbye packet");

    const status::StatusCode status = generate_packets_(current_time, PacketType_Goodbye);
    if (status != status::StatusOK) {
        roc_log(LogDebug, "rtcp communicator: generation failed: status=%s",
                status::code_to_str(status));
    }

    return status;
}

status::StatusCode Communicator::generate_packets_(core::nanoseconds_t current_time,
                                                   PacketType packet_type) {
    status::StatusCode status = begin_packet_generation_(current_time);
    if (status != status::StatusOK) {
        return status;
    }

    // Usually we generate one packet per destination address, however, if number of
    // streams is high, it may be split into multiple packets. We will continue
    // generation until all SR/RR and XR blocks are reported to all destination
    // addresses.
    while (continue_packet_generation_()) {
        packet::PacketPtr packet;
        status = generate_packet_(packet_type, packet);
        if (status != status::StatusOK) {
            break;
        }

        status = write_generated_packet_(packet);
        if (status != status::StatusOK) {
            break;
        }

        generated_packet_count_++;
    }

    status::StatusCode e_status = end_packet_generation_();
    if (status == status::StatusOK && e_status != status::StatusOK) {
        status = e_status;
    }

    log_stats_();

    return status;
}

status::StatusCode
Communicator::begin_packet_generation_(core::nanoseconds_t current_time) {
    const status::StatusCode status = reporter_.begin_generation(current_time);
    roc_log(LogTrace, "rtcp communicator: begin_generation(): status=%s",
            status::code_to_str(status));

    if (status != status::StatusOK) {
        return status;
    }

    dest_addr_count_ = 0;
    dest_addr_index_ = 0;

    send_stream_count_ = 0;
    send_stream_index_ = 0;

    recv_stream_count_ = 0;
    recv_stream_index_ = 0;

    return status::StatusOK;
}

status::StatusCode Communicator::end_packet_generation_() {
    const status::StatusCode status = reporter_.end_generation();
    roc_log(LogTrace, "rtcp communicator: end_generation(): status=%s",
            status::code_to_str(status));

    return status;
}

bool Communicator::continue_packet_generation_() {
    if (send_stream_index_ >= send_stream_count_
        && recv_stream_index_ >= recv_stream_count_) {
        if (dest_addr_count_ == 0) {
            // This is the very first report, do some initialization.
            dest_addr_count_ = reporter_.num_dest_addresses();
            dest_addr_index_ = 0;
        } else {
            // We've reported all blocks for current destination address,
            // switch to next address.
            roc_log(LogTrace,
                    "rtcp communicator: generated report: addr_index=%d addr_count=%d",
                    (int)dest_addr_index_, (int)dest_addr_count_);
            dest_addr_index_++;
        }

        if (dest_addr_index_ >= dest_addr_count_) {
            // We've reported all blocks for all destination addresses (or maybe
            // there are no destination addresses), exit generation.
            return false;
        }

        // Prepare to generate packets for new destination address.
        cur_pkt_send_stream_ = 0;
        cur_pkt_recv_stream_ = 0;

        send_stream_index_ = 0;
        send_stream_count_ =
            reporter_.is_sending() ? reporter_.num_sending_streams(dest_addr_index_) : 0;

        recv_stream_index_ = 0;
        recv_stream_count_ = reporter_.is_receiving()
            ? reporter_.num_receiving_streams(dest_addr_index_)
            : 0;
    }

    // Continue generation.
    return true;
}

status::StatusCode
Communicator::write_generated_packet_(const packet::PacketPtr& packet) {
    const status::StatusCode status = packet_writer_.write(packet);
    roc_log(LogTrace,
            "rtcp communicator: wrote packet:"
            " status=%s max_pkt_blocks=%d send_blocks=%d/%d recv_blocks=%d/%d",
            status::code_to_str(status), (int)max_pkt_streams_, (int)send_stream_index_,
            (int)send_stream_count_, (int)recv_stream_index_, (int)recv_stream_count_);

    return status;
}

bool Communicator::next_send_stream_(size_t new_stream_index) {
    // This function is called whenever we're going to add a report block for
    // the stream with given index. It checks whether it would lead to exceeding
    // the limit of streams per packet, and if not, updates the number of
    // streams in packet. It uses max() because it's called repeatedly for
    // the same streams, first for all streams when adding blocks of one type,
    // then for all streams when adding blocks of another type, and so on.
    const size_t new_pkt_send_stream =
        std::max(cur_pkt_send_stream_, new_stream_index - send_stream_index_ + 1);

    if (new_pkt_send_stream + cur_pkt_recv_stream_ >= max_pkt_streams_) {
        return false;
    }

    cur_pkt_send_stream_ = new_pkt_send_stream;
    return true;
}

bool Communicator::next_recv_stream_(size_t new_stream_index) {
    // See comment in next_send_stream_().
    const size_t next_pkt_recv_stream =
        std::max(cur_pkt_recv_stream_, new_stream_index - recv_stream_index_ + 1);

    if (cur_pkt_send_stream_ + next_pkt_recv_stream >= max_pkt_streams_) {
        return false;
    }

    cur_pkt_recv_stream_ = next_pkt_recv_stream;
    return true;
}

status::StatusCode Communicator::generate_packet_(PacketType packet_type,
                                                  packet::PacketPtr& packet) {
    packet = packet_factory_.new_packet();
    if (!packet) {
        roc_log(LogError, "rtcp communicator: can't create packet");
        return status::StatusNoMem;
    }

    // Buffer for RTCP packet data
    core::Slice<uint8_t> payload_buffer = packet_factory_.new_packet_buffer();
    if (!payload_buffer) {
        roc_log(LogError, "rtcp communicator: can't create buffer");
        return status::StatusNoMem;
    }
    payload_buffer.reslice(0, 0);

    // Fill RTCP packet data
    status::StatusCode status = generate_packet_payload_(packet_type, payload_buffer);
    if (status != status::StatusOK) {
        return status;
    }

    // Buffer for the whole packet. If RTCP composer is nested into another
    // composer, packet_data may hold additional headers or footers around
    // RTCP. If RTCP composer is the topmost, packet_data and rtcp_data
    // will be identical.
    core::Slice<uint8_t> packet_buffer = packet_factory_.new_packet_buffer();
    if (!packet_buffer) {
        roc_log(LogError, "rtcp communicator: can't create buffer");
        return status::StatusNoMem;
    }
    packet_buffer.reslice(0, 0);

    // Prepare packet to be able to hold our RTCP packet data
    status = packet_composer_.prepare(*packet, packet_buffer, payload_buffer.size());
    if (status != status::StatusOK) {
        roc_log(LogError, "rtcp communicator: can't prepare packet");
        return status;
    }
    packet->add_flags(packet::Packet::FlagPrepared);

    // Attach prepared packet buffer to the packet
    packet->set_buffer(packet_buffer);

    // prepare() call should have, among other things, set packet->rtcp()->data to a
    // sub-slice of packet_data, of size exactly as we requested
    if (!packet->rtcp() || !packet->rtcp()->payload
        || packet->rtcp()->payload.size() != payload_buffer.size()) {
        roc_panic("rtcp communicator: composer prepared invalid packet");
    }

    // Copy our RTCP packet data into that sub-slice
    memcpy(packet->rtcp()->payload.data(), payload_buffer.data(), payload_buffer.size());

    // Set destination address
    packet->add_flags(packet::Packet::FlagUDP);
    reporter_.generate_dest_address(dest_addr_index_, packet->udp()->dst_addr);

    return status::StatusOK;
}

status::StatusCode
Communicator::generate_packet_payload_(PacketType packet_type,
                                       core::Slice<uint8_t>& packet_payload) {
    for (;;) {
        // Start new packet.
        cur_pkt_send_stream_ = 0;
        cur_pkt_recv_stream_ = 0;

        Builder bld(config_, packet_payload);

        switch (packet_type) {
        case PacketType_Reports:
            generate_reports_payload_(bld);
            break;

        case PacketType_Goodbye:
            generate_goodbye_payload_(bld);
            break;
        }

        // Check if packet didn't fit into the buffer.
        if (!bld.is_ok()) {
            if (cur_pkt_send_stream_ + cur_pkt_recv_stream_ <= 1) {
                // Even one block can't fit into the buffer, so all we
                // can do is to report failure and exit.
                max_pkt_streams_ = 1;
                return status::StatusNoMem;
            }

            // Repeat current packet generation with reduced limit.
            // We will eventually either find value for max_pkt_blocks_
            // that does not cause errors, or report StatusNoMem (see above).
            // Normally this search will happen only once and then the
            // found value of max_pkt_blocks_ will be reused.
            max_pkt_streams_ = cur_pkt_send_stream_ + cur_pkt_recv_stream_ - 1;

            roc_log(LogTrace, "rtcp reporter: retrying generation with max_blocks=%lu",
                    (unsigned long)max_pkt_streams_);

            continue;
        }

        send_stream_index_ += cur_pkt_send_stream_;
        recv_stream_index_ += cur_pkt_recv_stream_;

        return status::StatusOK;
    }
}

void Communicator::generate_reports_payload_(Builder& bld) {
    // Add SR or RR.
    if (config_.enable_sr_rr) {
        generate_standard_report_(bld);
    }
    // Add XR.
    if (config_.enable_xr) {
        generate_extended_report_(bld);
    }
    // Add SDES.
    if (config_.enable_sdes) {
        generate_description_(bld);
    }
    // Add BYE in case of changing SSRC because of collision.
    if (reporter_.need_goodbye()) {
        generate_goodbye_(bld);
    }
}

void Communicator::generate_goodbye_payload_(Builder& bld) {
    // Add empty RR, as required by RFC 3550.
    if (config_.enable_sr_rr) {
        generate_empty_report_(bld);
    }
    // Add SDES, as required by RFC 3550.
    if (config_.enable_sdes) {
        generate_description_(bld);
    }
    // Add BYE.
    generate_goodbye_(bld);
}

void Communicator::generate_standard_report_(Builder& bld) {
    if (reporter_.is_sending()) {
        // We're either only sending, or sending + receiving.
        // Create SR in this case.
        header::SenderReportPacket sr;
        reporter_.generate_sr(sr);

        bld.begin_sr(sr);

        // If we're also receiving, add reception reports to SR.
        if (reporter_.is_receiving()) {
            for (size_t stream_index = recv_stream_index_;
                 stream_index < recv_stream_count_; stream_index++) {
                if (!next_recv_stream_(stream_index)) {
                    break;
                }

                header::ReceptionReportBlock blk;
                reporter_.generate_reception_block(dest_addr_index_, stream_index, blk);

                bld.add_sr_report(blk);
            }
        }

        bld.end_sr();
    } else {
        // We're either only receiving, or nor sending neither receiving.
        // Create RR in this case.
        header::ReceiverReportPacket rr;
        reporter_.generate_rr(rr);

        bld.begin_rr(rr);

        // If there are no actual receiving streams, keep RR empty,
        // as specified in RFC 3550.
        if (reporter_.is_receiving()) {
            for (size_t stream_index = recv_stream_index_;
                 stream_index < recv_stream_count_; stream_index++) {
                if (!next_recv_stream_(stream_index)) {
                    break;
                }

                header::ReceptionReportBlock blk;
                reporter_.generate_reception_block(dest_addr_index_, stream_index, blk);

                bld.add_rr_report(blk);
            }
        }

        bld.end_rr();
    }
}

void Communicator::generate_extended_report_(Builder& bld) {
    if ((reporter_.is_sending() && send_stream_index_ < send_stream_count_)
        || reporter_.is_receiving()) {
        header::XrPacket xr;
        reporter_.generate_xr(xr);

        bld.begin_xr(xr);

        if (reporter_.is_sending() && send_stream_index_ < send_stream_count_) {
            // DLRR is extended sender report.
            header::XrDlrrBlock dlrr;

            bld.begin_xr_dlrr(dlrr);

            for (size_t stream_index = send_stream_index_;
                 stream_index < send_stream_count_; stream_index++) {
                if (!next_send_stream_(stream_index)) {
                    break;
                }

                header::XrDlrrSubblock blk;
                reporter_.generate_dlrr_subblock(dest_addr_index_, stream_index, blk);

                bld.add_xr_dlrr_report(blk);
            }

            bld.end_xr_dlrr();
        }

        if (reporter_.is_receiving()) {
            // RRTR is extended receiver report.
            header::XrRrtrBlock rrtr;
            reporter_.generate_rrtr_block(rrtr);

            bld.add_xr_rrtr(rrtr);

            for (size_t stream_index = recv_stream_index_;
                 stream_index < recv_stream_count_; stream_index++) {
                if (!next_recv_stream_(stream_index)) {
                    break;
                }

                header::XrMeasurementInfoBlock mi_blk;
                reporter_.generate_measurement_info_block(dest_addr_index_, stream_index,
                                                          mi_blk);

                bld.add_xr_measurement_info(mi_blk);

                header::XrDelayMetricsBlock dm_blk;
                reporter_.generate_delay_metrics_block(dest_addr_index_, stream_index,
                                                       dm_blk);

                bld.add_xr_delay_metrics(dm_blk);

                header::XrQueueMetricsBlock qm_blk;
                reporter_.generate_queue_metrics_block(dest_addr_index_, stream_index,
                                                       qm_blk);

                bld.add_xr_queue_metrics(qm_blk);
            }
        }

        bld.end_xr();
    }
}

void Communicator::generate_empty_report_(Builder& bld) {
    header::ReceiverReportPacket rr;
    reporter_.generate_rr(rr);

    bld.begin_rr(rr);
    bld.end_rr();
}

void Communicator::generate_description_(Builder& bld) {
    bld.begin_sdes();

    {
        // Add single chunk with CNAME item.
        SdesChunk chunk;
        SdesItem item;
        reporter_.generate_cname(chunk, item);

        bld.begin_sdes_chunk(chunk);
        bld.add_sdes_item(item);
        bld.end_sdes_chunk();
    }

    bld.end_sdes();
}

void Communicator::generate_goodbye_(Builder& bld) {
    bld.begin_bye();

    {
        // Add single SSRC.
        packet::stream_source_t ssrc = 0;
        reporter_.generate_goodbye(ssrc);

        bld.add_bye_ssrc(ssrc);
    }

    bld.end_bye();
}

void Communicator::log_stats_() {
    if (!log_limiter_.allow()) {
        return;
    }

    roc_log(LogDebug,
            "rtcp communicator:"
            " generated_pkts=%lu processed_pkts=%lu proc_errs=%lu",
            (unsigned long)generated_packet_count_,
            (unsigned long)processed_packet_count_, (unsigned long)error_count_);

    error_count_ = 0;
    processed_packet_count_ = 0;
    generated_packet_count_ = 0;
}

} // namespace rtcp
} // namespace roc
