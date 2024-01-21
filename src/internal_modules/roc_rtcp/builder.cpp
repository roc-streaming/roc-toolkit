/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtcp/builder.h"
#include "roc_rtcp/headers.h"

namespace roc {
namespace rtcp {

Builder::Builder(const Config& config, core::Slice<uint8_t>& result)
    : state_(TOP)
    , result_slice_(result)
    , cur_pkt_slice_()
    , cur_pkt_header_(NULL)
    , cur_xr_block_header_(NULL)
    , sr_written_(false)
    , rr_written_(false)
    , cname_written_(false)
    , truncated_(false)
    , config_(config) {
    if (!result) {
        roc_panic("rtcp builder: slice is null");
    }
    result.reslice(0, 0);
}

Builder::~Builder() {
    if (state_ != TOP && state_ != LAST) {
        roc_panic("rtcp builder: wrong call order");
    }

    if (!truncated_ && result_slice_.size() == 0) {
        roc_panic("rtcp builder: packet can't be empty");
    }

    if (config_.enable_sr_rr && !sr_written_ && !rr_written_) {
        roc_panic("rtcp builder: packet should have SR or RR");
    }

    if (config_.enable_sdes && !cname_written_) {
        roc_panic("rtcp builder: packet should have CNAME");
    }
}

bool Builder::is_ok() const {
    return !truncated_;
}

void Builder::begin_sr(const header::SenderReportPacket& sr) {
    roc_panic_if_msg(state_ != TOP, "rtcp builder: wrong call order");

    roc_panic_if_msg(!config_.enable_sr_rr, "rtcp builder: SR is disabled");

    state_ = SR_HEAD;
    sr_written_ = true;

    header::SenderReportPacket* p =
        (header::SenderReportPacket*)begin_packet_(sizeof(sr));
    if (!p) {
        return;
    }
    memcpy(p, &sr, sizeof(sr));
}

void Builder::add_sr_report(const header::ReceptionReportBlock& report) {
    roc_panic_if_msg(state_ != SR_HEAD && state_ != SR_REPORT,
                     "rtcp builder: wrong call order");

    state_ = SR_REPORT;

    header::ReceptionReportBlock* p =
        (header::ReceptionReportBlock*)add_block_(sizeof(report));
    if (!p) {
        return;
    }
    memcpy(p, &report, sizeof(report));

    cur_pkt_header_->set_len_bytes(cur_pkt_slice_.size());
    cur_pkt_header_->inc_counter();
}

void Builder::end_sr() {
    roc_panic_if_msg(state_ != SR_HEAD && state_ != SR_REPORT,
                     "rtcp builder: wrong call order");

    state_ = TOP;

    end_packet_();
}

void Builder::begin_rr(const header::ReceiverReportPacket& rr) {
    roc_panic_if_msg(state_ != TOP, "rtcp builder: wrong call order");

    roc_panic_if_msg(!config_.enable_sr_rr, "rtcp builder: RR is disabled");

    state_ = RR_HEAD;
    rr_written_ = true;

    header::ReceiverReportPacket* p =
        (header::ReceiverReportPacket*)begin_packet_(sizeof(rr));
    if (!p) {
        return;
    }
    memcpy(p, &rr, sizeof(rr));
}

void Builder::add_rr_report(const header::ReceptionReportBlock& report) {
    roc_panic_if_msg(state_ != RR_HEAD && state_ != RR_REPORT,
                     "rtcp builder: wrong call order");

    state_ = RR_REPORT;

    header::ReceptionReportBlock* p =
        (header::ReceptionReportBlock*)add_block_(sizeof(report));
    if (!p) {
        return;
    }
    memcpy(p, &report, sizeof(report));

    cur_pkt_header_->set_len_bytes(cur_pkt_slice_.size());
    cur_pkt_header_->inc_counter();
}

void Builder::end_rr() {
    roc_panic_if_msg(state_ != RR_HEAD && state_ != RR_REPORT,
                     "rtcp builder: wrong call order");

    state_ = TOP;

    end_packet_();
}

void Builder::begin_sdes() {
    roc_panic_if_msg(state_ != TOP, "rtcp builder: wrong call order");

    roc_panic_if_msg(!config_.enable_sdes, "rtcp builder: SDES is disabled");

    roc_panic_if_msg(config_.enable_sr_rr && !sr_written_ && !rr_written_,
                     "rtcp builder: first packet should be SR or RR");

    state_ = SDES_HEAD;

    header::SdesPacket* p =
        (header::SdesPacket*)begin_packet_(sizeof(header::SdesPacket));
    if (!p) {
        return;
    }
    p->reset();
}

void Builder::begin_sdes_chunk(const SdesChunk& chunk) {
    roc_panic_if_msg(state_ != SDES_HEAD, "rtcp builder: wrong call order");

    state_ = SDES_CHUNK;
    cname_written_ = false;

    header::SdesChunkHeader* p =
        (header::SdesChunkHeader*)add_block_(sizeof(header::SdesChunkHeader));
    if (!p) {
        return;
    }
    p->reset();
    p->set_ssrc(chunk.ssrc);

    cur_pkt_header_->inc_counter();
}

void Builder::add_sdes_item(const SdesItem& item) {
    roc_panic_if_msg(state_ != SDES_CHUNK, "rtcp builder: wrong call order");

    roc_panic_if_msg(!item.text, "rtcp builder: SDES item text can't be null");

    const size_t text_size = strlen(item.text);
    const size_t total_size = sizeof(header::SdesItemHeader) + text_size;

    roc_panic_if_msg(text_size > header::MaxTextLen,
                     "rtcp builder: SDES item text can't longer than %d bytes",
                     (int)header::MaxTextLen);

    if (item.type == header::SDES_CNAME) {
        roc_panic_if_msg(item.text[0] == '\0',
                         "rtcp builder: CNAME item text can't be empty string");

        roc_panic_if_msg(
            cname_written_,
            "rtcp builder: each SDES chunk should have exactly one CNAME item");

        cname_written_ = true;
    }

    header::SdesItemHeader* p = (header::SdesItemHeader*)add_block_(total_size);
    if (!p) {
        return;
    }
    p->reset();
    p->set_type(item.type);
    p->set_text_len(text_size);

    if (text_size) {
        memcpy(p->text(), item.text, text_size);
    }
}

void Builder::end_sdes_chunk() {
    roc_panic_if_msg(state_ != SDES_CHUNK, "rtcp builder: wrong call order");

    roc_panic_if_msg(!cname_written_,
                     "rtcp builder: each SDES chunk should have exactly one CNAME item");

    state_ = SDES_HEAD;

    // Adds at least one byte with zero value and aligns the end with 32 bit border.
    const size_t padding_size = header::padding_len(cur_pkt_slice_.size(), 1);

    uint8_t* p = (uint8_t*)add_block_(padding_size);
    if (!p) {
        return;
    }
    memset(p, 0, padding_size);
}

void Builder::end_sdes() {
    roc_panic_if_msg(state_ != SDES_HEAD, "rtcp builder: wrong call order");

    state_ = TOP;

    end_packet_();
}

void Builder::begin_bye() {
    roc_panic_if_msg(state_ != TOP, "rtcp builder: wrong call order");

    roc_panic_if_msg(config_.enable_sr_rr && !sr_written_ && !rr_written_,
                     "rtcp builder: first packet should be SR or RR");

    state_ = BYE_HEAD;

    header::ByePacket* p = (header::ByePacket*)begin_packet_(sizeof(header::ByePacket));
    if (!p) {
        return;
    }
    p->reset();
}

void Builder::add_bye_ssrc(const packet::stream_source_t ssrc) {
    roc_panic_if_msg(state_ != BYE_HEAD && state_ != BYE_SSRC,
                     "rtcp builder: wrong call order");

    state_ = BYE_SSRC;

    header::ByeSourceHeader* p =
        (header::ByeSourceHeader*)add_block_(sizeof(header::ByeSourceHeader));
    if (!p) {
        return;
    }
    p->reset();
    p->set_ssrc(ssrc);

    cur_pkt_header_->inc_counter();
}

void Builder::add_bye_reason(const char* reason) {
    roc_panic_if_msg(state_ != BYE_SSRC, "rtcp builder: wrong call order");

    roc_panic_if_msg(!reason, "rtcp builder: BYE reason can't be null");

    const size_t text_size = strlen(reason);
    const size_t total_size = sizeof(header::ByeReasonHeader) + text_size;
    const size_t padding_size = header::padding_len(total_size, 0);

    roc_panic_if_msg(text_size > header::MaxTextLen,
                     "rtcp builder: BYE reason text can't longer than %d bytes",
                     (int)header::MaxTextLen);

    state_ = BYE_REASON;

    header::ByeReasonHeader* p =
        (header::ByeReasonHeader*)add_block_(total_size + padding_size);
    if (!p) {
        return;
    }
    p->reset();
    p->set_text_len(text_size);

    if (text_size) {
        memcpy(p->text(), reason, text_size);
    }
    if (padding_size) {
        memset(p->text() + text_size, 0, padding_size);
    }
}

void Builder::end_bye() {
    roc_panic_if_msg(state_ != BYE_SSRC && state_ != BYE_REASON,
                     "rtcp builder: wrong call order");

    state_ = TOP;

    end_packet_();
}

void Builder::begin_xr(const header::XrPacket& xr) {
    roc_panic_if_msg(state_ != TOP, "rtcp builder: wrong call order");

    roc_panic_if_msg(!config_.enable_xr, "rtcp builder: XR is disabled");

    roc_panic_if_msg(config_.enable_sr_rr && !sr_written_ && !rr_written_,
                     "rtcp builder: first packet should be SR or RR");

    state_ = XR_HEAD;

    header::XrPacket* p = (header::XrPacket*)begin_packet_(sizeof(xr));
    if (!p) {
        return;
    }
    memcpy(p, &xr, sizeof(xr));
}

void Builder::add_xr_rrtr(const header::XrRrtrBlock& rrtr) {
    roc_panic_if_msg(state_ != XR_HEAD, "rtcp builder: wrong call order");

    header::XrRrtrBlock* p = (header::XrRrtrBlock*)add_block_(sizeof(rrtr));
    if (!p) {
        return;
    }
    memcpy(p, &rrtr, sizeof(rrtr));

    cur_xr_block_header_ = &p->header();
    cur_xr_block_header_->set_len_bytes(sizeof(rrtr));
}

void Builder::begin_xr_dlrr(const header::XrDlrrBlock& dlrr) {
    roc_panic_if_msg(state_ != XR_HEAD, "rtcp builder: wrong call order");

    state_ = XR_DLRR_HEAD;

    header::XrDlrrBlock* p = (header::XrDlrrBlock*)add_block_(sizeof(dlrr));
    if (!p) {
        return;
    }
    memcpy(p, &dlrr, sizeof(dlrr));

    cur_xr_block_header_ = &p->header();
}

void Builder::add_xr_dlrr_report(const header::XrDlrrSubblock& report) {
    roc_panic_if_msg(state_ != XR_DLRR_HEAD && state_ != XR_DLRR_REPORT,
                     "rtcp builder: wrong call order");

    state_ = XR_DLRR_REPORT;

    header::XrDlrrSubblock* p = (header::XrDlrrSubblock*)add_block_(sizeof(report));
    if (!p) {
        return;
    }
    memcpy(p, &report, sizeof(report));
}

void Builder::end_xr_dlrr() {
    roc_panic_if_msg(state_ != XR_DLRR_HEAD && state_ != XR_DLRR_REPORT,
                     "rtcp builder: wrong call order");

    state_ = XR_HEAD;

    if (truncated_) {
        return;
    }

    cur_xr_block_header_->set_len_bytes(
        size_t(cur_pkt_slice_.data_end() - (uint8_t*)cur_xr_block_header_));
}

void Builder::add_xr_measurement_info(
    const header::XrMeasurementInfoBlock& measurement_info) {
    roc_panic_if_msg(state_ != XR_HEAD, "rtcp builder: wrong call order");

    header::XrMeasurementInfoBlock* p =
        (header::XrMeasurementInfoBlock*)add_block_(sizeof(measurement_info));
    if (!p) {
        return;
    }
    memcpy(p, &measurement_info, sizeof(measurement_info));

    cur_xr_block_header_ = &p->header();
    cur_xr_block_header_->set_len_bytes(sizeof(measurement_info));
}

void Builder::add_xr_delay_metrics(const header::XrDelayMetricsBlock& delay_metrics) {
    roc_panic_if_msg(state_ != XR_HEAD, "rtcp builder: wrong call order");

    header::XrDelayMetricsBlock* p =
        (header::XrDelayMetricsBlock*)add_block_(sizeof(delay_metrics));
    if (!p) {
        return;
    }
    memcpy(p, &delay_metrics, sizeof(delay_metrics));

    cur_xr_block_header_ = &p->header();
    cur_xr_block_header_->set_len_bytes(sizeof(delay_metrics));
}

void Builder::end_xr() {
    roc_panic_if_msg(state_ != XR_HEAD, "rtcp builder: wrong call order");

    state_ = TOP;

    end_packet_();
}

void Builder::add_padding(size_t padding_len) {
    roc_panic_if_msg(state_ != TOP, "rtcp builder: wrong call order");

    roc_panic_if_msg(padding_len % 4 != 0 || padding_len < 1 || padding_len > 255,
                     "rtcp builder: bad packet padding:"
                     " should be multiple of 4 in range [1; 255], got %lu",
                     (unsigned long)padding_len);

    state_ = LAST;

    if (truncated_) {
        return;
    }

    roc_panic_if_msg(!cur_pkt_header_,
                     "rtcp builder: can't add padding without adding packets");

    cur_pkt_header_->set_padding(true);
    cur_pkt_header_->set_len_bytes(cur_pkt_header_->len_bytes() + padding_len);

    if (result_slice_.capacity() - result_slice_.size() < padding_len) {
        truncated_ = true;
        return;
    }

    uint8_t* p = result_slice_.extend(padding_len);
    if (padding_len > 1) {
        memset(p, 0, padding_len - 1);
    }
    p[padding_len - 1] = (uint8_t)padding_len;
}

header::PacketHeader* Builder::begin_packet_(size_t size) {
    roc_panic_if_msg(size < sizeof(header::PacketHeader),
                     "rtcp builder: malfromed packet");

    if (truncated_) {
        return NULL;
    }

    cur_pkt_slice_ = result_slice_.subslice(result_slice_.size(), result_slice_.size());

    if (cur_pkt_slice_.capacity() - cur_pkt_slice_.size() < size) {
        truncated_ = true;
        return NULL;
    }

    cur_pkt_header_ = (header::PacketHeader*)cur_pkt_slice_.extend(size);

    return cur_pkt_header_;
}

void* Builder::add_block_(size_t size) {
    if (truncated_) {
        return NULL;
    }

    if (cur_pkt_slice_.capacity() - cur_pkt_slice_.size() < size) {
        truncated_ = true;
        return NULL;
    }

    return cur_pkt_slice_.extend(size);
}

void Builder::end_packet_() {
    if (truncated_) {
        return;
    }

    roc_panic_if_msg(cur_pkt_slice_.size() < sizeof(header::PacketHeader)
                         || cur_pkt_slice_.size() % 4 != 0,
                     "rtcp builder: malfromed packet");

    cur_pkt_header_->set_len_bytes(cur_pkt_slice_.size());
    result_slice_.extend(cur_pkt_slice_.size());
}

} // namespace rtcp
} // namespace roc
