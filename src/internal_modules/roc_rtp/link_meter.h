/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/link_meter.h
//! @brief Link meter.

#ifndef ROC_RTP_LINK_METER_H_
#define ROC_RTP_LINK_METER_H_

#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/ireader.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"
#include "roc_rtp/encoding.h"
#include "roc_rtp/encoding_map.h"

namespace roc {
namespace rtp {

//! Link metrics.
struct LinkMetrics {
    //! Number of RTP timestamp units per second.
    size_t sample_rate;

    //! Extended lowest RTP seqnum received.
    //! The low 16 bits contain the lowest sequence number received in an RTP data
    //! packet, and the rest bits extend that sequence number with the corresponding
    //! count of seqnum cycles.
    packet::ext_seqnum_t ext_first_seqnum;

    //! Extended highest RTP seqnum received.
    //! The low 16 bits contain the highest sequence number received in an RTP data
    //! packet, and the rest bits extend that sequence number with the corresponding
    //! count of seqnum cycles.
    packet::ext_seqnum_t ext_last_seqnum;

    //! Fraction of lost packets from 0 to 1.
    //! The fraction of RTP data packets lost since the previous report was sent.
    //! Defined to be the number of packets lost divided by the number of packets
    //! expected. If the loss is negative due to duplicates, set to zero.
    float fract_loss;

    //! Cumulative count of lost packets.
    //! The total number of RTP data packets that have been lost since the beginning
    //! of reception. Defined to be the number of packets expected minus the number of
    //! packets actually received, where the number of packets received includes any
    //! which are late or duplicates. Packets that arrive late are not counted as lost,
    //! and the loss may be negative if there are duplicates.
    long cum_loss;

    //! Estimated interarrival jitter.
    //! An estimate of the statistical variance of the RTP data packet
    //! interarrival time.
    core::nanoseconds_t jitter;

    LinkMetrics()
        : sample_rate(0)
        , ext_first_seqnum(0)
        , ext_last_seqnum(0)
        , fract_loss(0)
        , cum_loss(0)
        , jitter(0) {
    }
};

//! Link meter.
//!
//! Computes various link metrics based on sequence of RTP packets.
//! Inserted into pipeline in two points:
//!
//!  - As a writer, right after receiving packet, before storing
//!    packet in incoming queue. Here LinkMeter computes metrics
//!    that should be updated as early as possible.
//!
//!  - As a reader, right before decoding packet. Here LinkMeter
//!    computes metrics that can be updated only when packets
//!    are going to be played.
//!
//! In both cases, LinkMeter passes through packets to/from nested
//! writer/reader, and updates metrics.
class LinkMeter : public packet::IWriter,
                  public packet::IReader,
                  public core::NonCopyable<> {
public:
    //! Initialize.
    LinkMeter(const EncodingMap& encoding_map);

    //! Write packet and update metrics.
    //! @remarks
    //!  Invoked early in pipeline right after the packet is received.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr& packet);

    //! Read packet and update metrics.
    //! @remarks
    //!  Invoked late in pipeline right before the packet is decoded.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(packet::PacketPtr& packet);

    //! Set nested packet writer.
    //! @remarks
    //!  Should be called before first write() call.
    void set_writer(packet::IWriter& writer);

    //! Set nested packet reader.
    //! @remarks
    //!  Should be called before first read() call.
    void set_reader(packet::IReader& reader);

    //! Check if metrics are already gathered and can be reported.
    bool has_metrics() const;

    //! Get metrics.
    LinkMetrics metrics() const;

private:
    void update_metrics_(const packet::Packet& packet);

    const EncodingMap& encoding_map_;
    const Encoding* encoding_;

    packet::IWriter* writer_;
    packet::IReader* reader_;

    bool first_packet_;
    bool has_metrics_;

    LinkMetrics metrics_;

    uint16_t first_seqnum_;
    uint32_t last_seqnum_hi_;
    uint16_t last_seqnum_lo_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_LINK_METER_H_
