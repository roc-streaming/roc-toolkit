/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/builder.h
//! @brief RTCP packet builder.

#ifndef ROC_RTCP_BUILDER_H_
#define ROC_RTCP_BUILDER_H_

#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"
#include "roc_rtcp/bye_traverser.h"
#include "roc_rtcp/config.h"
#include "roc_rtcp/headers.h"
#include "roc_rtcp/sdes.h"

namespace roc {
namespace rtcp {

//! RTCP compound packet builder.
//!
//! Builder will panic if any of the following rules is violated
//! (mandated by RFC 3550):
//!  - At least one packet should be present.
//!  - First packet should be SR or RR.
//!  - SDES packet with CNAME item should be present.
//!  - Each SDES chunk should have exactly one CNAME item.
//!  - Padding can be added only to last packet.
//!
//! If the packet does not fit into resulting slice, builder will raise
//! error flags, and all its method will become no-op.
//!
//! Some of these rules may be disabled via config struct, which is used
//! in tests when we need to produce not strictly correct RTCP packets.
class Builder : public core::NonCopyable<> {
public:
    //! Initialize builder.
    //! It will write data to the given slice.
    Builder(const Config& config, core::Slice<uint8_t>& result);
    ~Builder();

    //! Check for errors.
    //! @returns false if the packet did not fit into the slice.
    bool is_ok() const;

    //! @name Sender Report (SR)
    //! @{

    //! Start SR packet inside compound RTCP packet.
    void begin_sr(const header::SenderReportPacket& sr);

    //! Add reception report to current SR packet.
    void add_sr_report(const header::ReceptionReportBlock& report);

    //! Finish SR packet.
    void end_sr();

    //! @}

    //! @name Receiver Report (RR)
    //! @{

    //! Start RR packet inside compound RTCP packet.
    void begin_rr(const header::ReceiverReportPacket& rr);

    //! Add reception report to current RR packet.
    void add_rr_report(const header::ReceptionReportBlock& report);

    //! Finish RR packet.
    void end_rr();

    //! @}

    //! @name Extended Report (XR)
    //! @{

    //! Start XR packet inside compound RTCP packet.
    void begin_xr(const header::XrPacket& xr);

    //! Add RRTR block to current XR packet.
    void add_xr_rrtr(const header::XrRrtrBlock& rrtr);

    //! Start DLRR block inside current XR packet.
    void begin_xr_dlrr(const header::XrDlrrBlock& dlrr);

    //! Add DLRR report to current DLRR block.
    void add_xr_dlrr_report(const header::XrDlrrSubblock& report);

    //! Add measurement info block.to current XR packet.
    void add_xr_measurement_info(const header::XrMeasurementInfoBlock& measurement_info);

    //! Add delay metrics block.to current XR packet.
    void add_xr_delay_metrics(const header::XrDelayMetricsBlock& delay_metrics);

    //! Finish current DLRR block.
    void end_xr_dlrr();

    //! Finish current XR packet.
    void end_xr();

    //! @}

    //! @name Session Description (SDES)
    //! @{

    //! Start SDES packet inside compound RTCP packet.
    void begin_sdes();

    //! Start new SDES chunk in current SDES packet.
    void begin_sdes_chunk(const SdesChunk& chunk);

    //! Add SDES item to current SDES chunk.
    void add_sdes_item(const SdesItem& item);

    //! Finish current SDES chunk.
    void end_sdes_chunk();

    //! Finish current SDES packet.
    void end_sdes();

    //! @}

    //! @name Goodbye message (BYE)
    //! @{

    //! Start BYE packet inside compound RTCP packet.
    void begin_bye();

    //! Add SSRC to current BYE packet.
    void add_bye_ssrc(const packet::stream_source_t ssrc);

    //! Add REASON to current BYE packet.
    void add_bye_reason(const char* reason);

    //! Finish current BYE packet.
    void end_bye();

    //! @}

    //! @name Packet padding
    //! @{

    //! Add given number of padding bytes to last packet.
    //! Padding should be multiple of 4 in range [1; 255].
    void add_padding(size_t padding_len);

    //! @}

private:
    header::PacketHeader* begin_packet_(size_t size);
    void* add_block_(size_t size);
    void end_packet_();

    enum State {
        TOP,
        SR_HEAD,
        SR_REPORT,
        RR_HEAD,
        RR_REPORT,
        XR_HEAD,
        XR_DLRR_HEAD,
        XR_DLRR_REPORT,
        SDES_HEAD,
        SDES_CHUNK,
        BYE_HEAD,
        BYE_SSRC,
        BYE_REASON,
        LAST
    };

    State state_;

    core::Slice<uint8_t>& result_slice_;

    core::Slice<uint8_t> cur_pkt_slice_;
    header::PacketHeader* cur_pkt_header_;
    header::XrBlockHeader* cur_xr_block_header_;

    bool sr_written_;
    bool rr_written_;
    bool cname_written_;

    bool truncated_;

    const Config config_;
};

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_BUILDER_H_
