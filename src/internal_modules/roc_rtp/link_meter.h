/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/link_meter.h
//! @brief RTP link meter.

#ifndef ROC_RTP_LINK_METER_H_
#define ROC_RTP_LINK_METER_H_

#include "roc_audio/jitter_meter.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_dbgio/csv_dumper.h"
#include "roc_packet/ilink_meter.h"
#include "roc_packet/iwriter.h"
#include "roc_rtcp/reports.h"
#include "roc_rtp/encoding.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace rtp {

//! RTP link meter.
//!
//! Computes various link metrics based on sequence of RTP packets.
//!
//! Inserted into pipeline as a writer, right after receiving packet, before storing
//! packet in incoming queue, which allows to update metrics as soon as new packets
//! arrive, without waiting until it's requested by depacketizer.
class LinkMeter : public packet::ILinkMeter,
                  public packet::IWriter,
                  public core::NonCopyable<> {
public:
    //! Initialize.
    LinkMeter(packet::IWriter& writer,
              const audio::JitterMeterConfig& jitter_config,
              const EncodingMap& encoding_map,
              core::IArena& arena,
              dbgio::CsvDumper* dumper);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Check if metrics are already gathered and can be reported.
    virtual bool has_metrics() const;

    //! Get metrics.
    virtual const packet::LinkMetrics& metrics() const;

    //! Check if packet encoding already detected.
    bool has_encoding() const;

    //! Get detected encoding.
    //! @remarks
    //!  Panics if no encoding detected.
    const Encoding& encoding() const;

    //! Process RTCP report from sender.
    //! @remarks
    //!  Obtains additional information that can't be measured directly.
    void process_report(const rtcp::SendReport& report);

    //! Write packet and update metrics.
    //! @remarks
    //!  Invoked early in pipeline right after the packet is received.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr& packet);

private:
    void update_metrics_(const packet::Packet& packet);

    void update_seqnums_(const packet::Packet& packet);
    void update_jitter_(const packet::Packet& packet);

    void dump_(const packet::Packet& packet);

    const EncodingMap& encoding_map_;
    const Encoding* encoding_;

    packet::IWriter& writer_;

    bool first_packet_;

    bool has_metrics_;
    packet::LinkMetrics metrics_;

    uint16_t first_seqnum_;
    uint32_t last_seqnum_hi_;
    uint16_t last_seqnum_lo_;

    int64_t processed_packets_;
    core::nanoseconds_t prev_queue_timestamp_;
    packet::stream_timestamp_t prev_stream_timestamp_;

    audio::JitterMeter jitter_meter_;

    dbgio::CsvDumper* dumper_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_LINK_METER_H_
