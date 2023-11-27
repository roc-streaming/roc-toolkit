/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/headers.h
//! @brief RTCP headers.

#ifndef ROC_RTCP_HEADERS_H_
#define ROC_RTCP_HEADERS_H_

#include "roc_core/attributes.h"
#include "roc_core/endian.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_packet/ntp.h"
#include "roc_packet/units.h"

namespace roc {
namespace rtcp {
namespace header {

//! Set some bits in v0.
//! @param v0 Where to write the bits.
//! @param v1 The bits to write.
//! @param shift From which bit num the field start.
//! @param mask The bitmask.
template <typename T>
void set_bitfield(T& v0, const T v1, const size_t shift, const size_t mask) {
    v0 &= T(~(mask << shift));
    v0 |= T(v1 << shift);
}

//! Computes the value of RTCP packet header length field from input number.
inline uint16_t size_t_2_rtcp_length(const size_t x) {
    roc_panic_if(x < 4);
    roc_panic_if(x > uint16_t(-1));
    roc_panic_if(x % 4 != 0);
    return (uint16_t)x / 4 - 1;
}

//! Converts RTCP header length field into conventional size_t value.
inline size_t rtcp_length_2_size_t(const size_t x) {
    return (x + 1) * 4;
}

//! How much padding bytes do we need in order to align with 32-bits.
//! @param size defines data length in bytes.
//! @param min_padding defines minimum number of padding bytes required.
//! @return How much should be added to x.
inline size_t padding_len(const size_t size, const size_t min_padding) {
    const size_t size_to_pad = size + min_padding;
    return min_padding + (size_to_pad & 0x03 ? 4 - (size_to_pad & 0x03) : 0);
}

//! Get a block that follows header, by index.
template <class Blk, class Pkt>
Blk& get_block_by_index(Pkt* pkt,
                        size_t block_index,
                        size_t num_blocks,
                        const char* pkt_type) {
    if (block_index >= num_blocks) {
        roc_panic("%s: out of bounds: index=%lu size=%lu", pkt_type,
                  (unsigned long)block_index, (unsigned long)num_blocks);
    }
    return ((Blk*)(const_cast<char*>((const char*)pkt) + sizeof(*pkt)))[block_index];
}

//! RTP protocol version.
enum Version {
    V2 = 2 //!< RTP version 2.
};

//! RTCP packet type.
enum PacketType {
    RTCP_SR = 200,   //!< Sender report packet.
    RTCP_RR = 201,   //!< Receiver report packet.
    RTCP_SDES = 202, //!< Source Description packet.
    RTCP_BYE = 203,  //!< BYE packet.
    RTCP_APP = 204,  //!< APP-specific packet.
    RTCP_XR = 207    //!< Extended report packet.
};

//! Maximum number of inner blocks/chunks in RTCP packet.
static const size_t PacketMaxBlocks = 31;

//! Helper to store 64-bit ntp timestamp in a common way among RTCP.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |              NTP timestamp, most significant word             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |             NTP timestamp, least significant word             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class NtpTimestamp {
private:
    enum {
        High_shift = 32,
        High_mask = 0xFFFFFFFF00000000,

        Low_shift = 0,
        Low_mask = 0x00000000FFFFFFFF
    };

    uint32_t high_;
    uint32_t low_;

public:
    NtpTimestamp() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        high_ = 0;
        low_ = 0;
    }

    //! Get NTP timestamp value.
    packet::ntp_timestamp_t value() const {
        const uint64_t res = (((uint64_t)core::ntoh32u(high_) << High_shift) & High_mask)
            | (((uint64_t)core::ntoh32u(low_) << Low_shift) & Low_mask);

        return (packet::ntp_timestamp_t)res;
    }

    //! Set NTP timestamp value.
    void set_value(const packet::ntp_timestamp_t t) {
        high_ = core::hton32u(uint32_t((t >> High_shift) & Low_mask));
        low_ = core::hton32u(uint32_t((t >> Low_shift) & Low_mask));
    }
} ROC_ATTR_PACKED_END;

//! RTCP packet header, common for all RTCP packet types.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |V=2|P|    RC   |   PT=SR=200   |             length            |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class PacketHeader {
private:
    enum {
        //! @name RTCP protocol version.
        // @{
        Flag_VersionShift = 6,
        Flag_VersionMask = 0x03,
        // @}

        //! @name RTCP padding flag.
        // @{
        Flag_PaddingShift = 5,
        Flag_PaddingMask = 0x01,
        // @}

        //! @name RTCP packets counter.
        // @{
        Flag_CounterShift = 0,
        Flag_CounterMask = 0x1F
        // @}
    };

    //! Protocol version, padding flag, and block/chunk counter.
    //! Varies by packet type.
    uint8_t count_;

    //! RTCP packet type.
    uint8_t type_;

    //! Packet length in 4-byte words, w/o common packet header word.
    uint16_t length_;

public:
    PacketHeader() {
        reset(PacketType(0));
    }

    //! Reset to initial state (all zeros).
    void reset(const PacketType t) {
        count_ = 0;
        type_ = 0;
        length_ = 0;

        set_version(V2);
        set_type(t);
    }

    //! Get number of blocks/chunks following.
    size_t counter() const {
        return (count_ >> Flag_CounterShift) & Flag_CounterMask;
    }

    //! Set number of blocks/chunks.
    void set_counter(const size_t c) {
        roc_panic_if(c > PacketMaxBlocks);
        set_bitfield<uint8_t>(count_, (uint8_t)c, Flag_CounterShift, Flag_CounterMask);
    }

    //! Increment packet counter,
    void inc_counter() {
        return set_counter(counter() + 1);
    }

    //! Get protocol version.
    uint8_t version() const {
        return (count_ >> Flag_VersionShift) & Flag_VersionMask;
    }

    //! Set protocol version.
    void set_version(Version v) {
        roc_panic_if((v & Flag_VersionMask) != v);
        set_bitfield<uint8_t>(count_, v, Flag_VersionShift, Flag_VersionMask);
    }

    //! Get padding flag.
    bool has_padding() const {
        return (count_ & (Flag_PaddingMask << Flag_PaddingShift));
    }

    //! Set padding flag.
    void set_padding(bool v) {
        set_bitfield(count_, (uint8_t)v, Flag_PaddingShift, Flag_PaddingMask);
    }

    //! Get packet type.
    PacketType type() const {
        return PacketType(type_);
    }

    //! Set packet type.
    void set_type(const PacketType t) {
        roc_panic_if_not(t == 0 || (t >= RTCP_SR && t <= RTCP_XR));
        type_ = t;
    }

    //! Get packet length, including the header, in 32-bit words minus one.
    uint16_t len_words() const {
        return core::ntoh16u(length_);
    }

    //! Set packet length in words.
    void set_len_words(const uint16_t len) {
        length_ = core::hton16u(len);
    }

    //! Get packet length, including the header, in bytes.
    size_t len_bytes() const {
        return rtcp_length_2_size_t(len_words());
    }

    //! Set packet length in bytes.
    void set_len_bytes(const size_t len) {
        set_len_words(size_t_2_rtcp_length(len));
    }
} ROC_ATTR_PACKED_END;

//! Reception report block.
//!
//! Part of RR and SR packets.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |                             SSRC                              |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! | fraction lost |       cumulative number of packets lost       |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |           extended highest sequence number received           |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                      interarrival jitter                      |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                         last SR (LSR)                         |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                   delay since last SR (DLSR)                  |
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//!  @endcode
ROC_ATTR_PACKED_BEGIN class ReceptionReportBlock {
private:
    enum {
        //! @name Fraction lost since last SR/RR.
        // @{
        Losses_FractLost_shift = 24,
        Losses_FractLoss_width = 8,
        Losses_FractLost_mask = 0xFF,
        // @}

        //! @name Cumulative number of packets lost since the beginning.
        // @{
        Losses_CumLoss_shift = 0,
        Losses_CumLoss_width = 24,
        Losses_CumLoss_mask = 0xFFFFFF
        // @}
    };

    //! Data source being reported.
    uint32_t ssrc_;

    //! Fraction lost since last SR/RR and cumulative number of
    //! packets lost since the beginning of reception (signed!).
    uint32_t losses_;

    //! Extended last seq. no. received.
    uint32_t last_seq_;

    //! Interarrival jitter.
    uint32_t jitter_;

    //! Last SR packet from this source.
    uint32_t last_sr_;

    //! Delay since last SR packet.
    uint32_t delay_last_sr_;

public:
    ReceptionReportBlock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        ssrc_ = losses_ = last_seq_ = jitter_ = last_sr_ = delay_last_sr_ = 0;
    }

    //! Get SSRC.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC.
    void set_ssrc(const packet::stream_source_t s) {
        ssrc_ = core::hton32u(s);
    }

    //! Get fraction lost.
    float fract_loss() const {
        const uint32_t losses = core::ntoh32u(losses_);
        const uint8_t fract_loss8 =
            (losses >> Losses_FractLost_shift) & Losses_FractLost_mask;
        const float res = float(fract_loss8) / float(1 << Losses_FractLoss_width);

        return res;
    }

    //! Set fractional loss.
    //! Fractional loss is stored in Q.8 format.
    void set_fract_loss(float fract_loss) {
        if (fract_loss > 1) {
            fract_loss = 1;
        }
        if (fract_loss < 0) {
            fract_loss = 0;
        }

        const uint8_t fract_loss8 =
            (uint8_t)(uint32_t)(fract_loss * float(1 << Losses_FractLoss_width));

        uint32_t losses = core::ntoh32u(losses_);
        set_bitfield<uint32_t>(losses, fract_loss8, Losses_FractLost_shift,
                               Losses_FractLost_mask);

        losses_ = core::hton32u(losses);
    }

    //! Get cumulative loss.
    //! May be negative in case of packet duplications.
    int32_t cum_loss() const {
        uint32_t res =
            (core::ntoh32u(losses_) >> Losses_CumLoss_shift) & Losses_CumLoss_mask;
        // If res is negative
        if (res & (1 << (Losses_CumLoss_width - 1))) {
            // Make whole leftest byte filled with 1.
            res |= ~(uint32_t)Losses_CumLoss_mask;
        }
        return (int32_t)res;
    }

    //! Set cumulative loss.
    //! May be negative in case of packet duplications.
    void set_cum_loss(int32_t cum_loss) {
        if (cum_loss > Losses_CumLoss_mask) {
            cum_loss = Losses_CumLoss_mask;
        } else if (cum_loss < -(int32_t)Losses_CumLoss_mask) {
            cum_loss = -Losses_CumLoss_mask;
        }

        uint32_t losses = core::ntoh32u(losses_);
        set_bitfield<uint32_t>(losses, (uint32_t)cum_loss, Losses_CumLoss_shift,
                               Losses_CumLoss_mask);

        losses_ = core::hton32u(losses);
    }

    //! Get last seqnum.
    uint32_t last_seqnum() const {
        return core::ntoh32u(last_seq_);
    }

    //! Set last seqnum.
    void set_last_seqnum(const uint32_t x) {
        last_seq_ = core::hton32u(x);
    }

    //! Get jitter.
    packet::stream_timestamp_t jitter() const {
        return core::ntoh32u(jitter_);
    }

    //! Set jitter.
    void set_jitter(const packet::stream_timestamp_t x) {
        jitter_ = core::hton32u(x);
    }

    //! Get LSR.
    packet::ntp_timestamp_t last_sr() const {
        packet::ntp_timestamp_t x = core::ntoh32u(last_sr_);
        x <<= 16;
        return x;
    }

    //! Set LSR.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_last_sr(packet::ntp_timestamp_t x) {
        x >>= 16;
        x &= 0xffffffff;
        last_sr_ = core::hton32u((uint32_t)x);
    }

    //! Get DLSR.
    packet::ntp_timestamp_t delay_last_sr() const {
        return core::ntoh32u(delay_last_sr_);
    }

    //! Set DLSR.
    //! Stores only the low 32 bits out of 64 in the NTP timestamp.
    void set_delay_last_sr(packet::ntp_timestamp_t x) {
        x &= 0xffffffff;
        delay_last_sr_ = core::hton32u((uint32_t)x);
    }
} ROC_ATTR_PACKED_END;

//! Receiver Report RTCP packet (RR).
//!
//! RFC 3550 6.4.2. "RR: Receiver Report RTCP packet"
//!
//! @code
//!         0                   1                   2                   3
//!         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! header |V=2|P|    RC   |   PT=RR=201   |             length            |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                     SSRC of packet sender                     |
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! report |                 SSRC_1 (SSRC of first source)                 |
//! block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   1    | fraction lost |       cumulative number of packets lost       |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |           extended highest sequence number received           |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                      interarrival jitter                      |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                         last SR (LSR)                         |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                   delay since last SR (DLSR)                  |
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! report |                 SSRC_2 (SSRC of second source)                |
//! block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   2    :                               ...                             :
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//!        |                  profile-specific extensions                  |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!  @endcode
ROC_ATTR_PACKED_BEGIN class ReceiverReportPacket {
private:
    PacketHeader header_;

    //! Data source being reported.
    uint32_t ssrc_;

public:
    ReceiverReportPacket() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(RTCP_RR);
        ssrc_ = 0;
    }

    //! Get common packet header.
    const PacketHeader& header() const {
        return header_;
    }

    //! Get common packet header.
    PacketHeader& header() {
        return header_;
    }

    //! Get SSRC of packet sender.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC of packet sender.
    void set_ssrc(const packet::stream_source_t s) {
        ssrc_ = core::hton32u(s);
    }

    //! Get number of blocks.
    size_t num_blocks() const {
        return header_.counter();
    }

    //! Get reception block by index.
    const ReceptionReportBlock& get_block(const size_t i) const {
        return get_block_by_index<const ReceptionReportBlock>(this, i, header().counter(),
                                                              "rtcp rr");
    }

    //! Get reception block by index.
    ReceptionReportBlock& get_block(const size_t i) {
        return get_block_by_index<ReceptionReportBlock>(this, i, header().counter(),
                                                        "rtcp rr");
    }
} ROC_ATTR_PACKED_END;

//! Sender Report RTCP packet (SR).
//!
//! RFC 3550 6.4.1. "SR: Sender Report RTCP packet"
//!
//! @code
//!         0                   1                   2                   3
//!         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! header |V=2|P|    RC   |   PT=SR=200   |             length            |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                         SSRC of sender                        |
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! sender |              NTP timestamp, most significant word             |
//! info   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |             NTP timestamp, least significant word             |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                         RTP timestamp                         |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                     sender's packet count                     |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                      sender's octet count                     |
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! report |                 SSRC_1 (SSRC of first source)                 |
//! block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   1    | fraction lost |       cumulative number of packets lost       |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |           extended highest sequence number received           |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                      interarrival jitter                      |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                         last SR (LSR)                         |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!        |                   delay since last SR (DLSR)                  |
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! report |                 SSRC_2 (SSRC of second source)                |
//! block  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!   2    :                               ...                             :
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//!        |                  profile-specific extensions                  |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class SenderReportPacket {
private:
    PacketHeader header_;

    uint32_t ssrc_;
    NtpTimestamp ntp_timestamp_;
    uint32_t rtp_timestamp_;
    uint32_t packet_cnt_;
    uint32_t bytes_cnt_;

public:
    SenderReportPacket() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(RTCP_SR);
        ssrc_ = 0;
        ntp_timestamp_.reset();
        rtp_timestamp_ = 0;
        packet_cnt_ = 0;
        bytes_cnt_ = 0;
    }

    //! Get common packet header.
    const PacketHeader& header() const {
        return header_;
    }

    //! Get common packet header.
    PacketHeader& header() {
        return header_;
    }

    //! Get SSRC of sender.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC of sender.
    void set_ssrc(const packet::stream_source_t s) {
        ssrc_ = core::hton32u(s);
    }

    //! Get NTP timestamp.
    packet::ntp_timestamp_t ntp_timestamp() const {
        return ntp_timestamp_.value();
    }

    //! Set NTP timestamp.
    void set_ntp_timestamp(const packet::ntp_timestamp_t t) {
        ntp_timestamp_.set_value(t);
    }

    //! Get RTP timestamp.
    packet::stream_timestamp_t rtp_timestamp() const {
        return core::ntoh32u(rtp_timestamp_);
    }

    //! Get RTP timestamp.
    void set_rtp_timestamp(const packet::stream_timestamp_t t) {
        rtp_timestamp_ = core::hton32u(t);
    }

    //! Get packet count.
    uint32_t packet_count() const {
        return core::ntoh32u(packet_cnt_);
    }

    //! Set packet count.
    void set_packet_count(const uint32_t cnt) {
        packet_cnt_ = core::hton32u(cnt);
    }

    //! Get byte count.
    uint32_t byte_count() const {
        return core::ntoh32u(bytes_cnt_);
    }

    //! Set byte count.
    void set_byte_count(const uint32_t cnt) {
        bytes_cnt_ = core::hton32u(cnt);
    }

    //! Get number of blocks.
    size_t num_blocks() const {
        return header_.counter();
    }

    //! Get reception block by index.
    const ReceptionReportBlock& get_block(const size_t i) const {
        return get_block_by_index<const ReceptionReportBlock>(this, i, header().counter(),
                                                              "rtcp sr");
    }

    //! Get reception block by index.
    ReceptionReportBlock& get_block(const size_t i) {
        return get_block_by_index<ReceptionReportBlock>(this, i, header().counter(),
                                                        "rtcp sr");
    }
} ROC_ATTR_PACKED_END;

//! SDES item type.
enum SdesItemType {
    SDES_CNAME = 1, //!< Canonical End-Point Identifier.
    SDES_NAME = 2,  //!< User Name.
    SDES_EMAIL = 3, //!< Electronic Mail Address.
    SDES_PHONE = 4, //!< Phone Number.
    SDES_LOC = 5,   //!< Geographic User Location.
    SDES_TOOL = 6,  //!< Application or Tool Name.
    SDES_NOTE = 7,  //!< Notice/Status.
    SDES_PRIV = 8   //!< Private Extensions.
};

//! SDES chunk header.
//!
//! Part of SDES packet.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |                              SSRC                             |
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! @endcode
ROC_ATTR_PACKED_BEGIN class SdesChunkHeader {
private:
    uint32_t ssrc_;

public:
    SdesChunkHeader() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        ssrc_ = 0;
    }

    //! Get SSRC.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC.
    void set_ssrc(const packet::stream_source_t s) {
        ssrc_ = core::hton32u(s);
    }
} ROC_ATTR_PACKED_END;

//! SDES item header.
//!
//! Part of SDES packet.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |     Type      |   Length      | Text  in UTF-8              ...
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! @endcode
ROC_ATTR_PACKED_BEGIN class SdesItemHeader {
private:
    uint8_t type_;
    uint8_t len_;

public:
    //! Get maximum allowed item text length.
    static const size_t MaxTextLen = 255;

    SdesItemHeader() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        type_ = len_ = 0;
    }

    //! Get item type.
    SdesItemType type() const {
        return SdesItemType(type_);
    }

    //! Set type.
    void set_type(const SdesItemType t) {
        roc_panic_if_not(t == 0 || (t >= SDES_CNAME && t <= SDES_PRIV));
        type_ = t;
    }

    //! Get item text length.
    size_t text_len() const {
        return len_;
    }

    //! Set item text length.
    void set_text_len(const size_t len) {
        roc_panic_if(len > MaxTextLen);
        len_ = (uint8_t)len;
    }

    //! Get pointer to item text.
    //! The text is NOT zero-terminated.
    const uint8_t* text() const {
        return (const uint8_t*)this + sizeof(*this);
    }

    //! Get pointer to item text.
    //! The text is NOT zero-terminated.
    uint8_t* text() {
        return (uint8_t*)this + sizeof(*this);
    }
} ROC_ATTR_PACKED_END;

//! Source Description RTCP packet (SDES).
//!
//! RFC 3550 6.5. "SDES: Source Description RTCP packet"
//!
//! @code
//!         0                   1                   2                   3
//!         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! header |V=2|P|    RC   |   PT=Sdes=202  |             length           |
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! Chunk1 |                 SSRC_1 (SSRC of first source)                 |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! Item1  |     Type      |   Length      | Text  in UTF-8                |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! Item2  |     Type      |   Length      | Text  in UTF-8                |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! Item 3 |     Type      |   Length      | Text  in UTF-8                |
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! Chunk2 |                 SSRC_2 (SSRC of second source)                |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! Item1  |     Type      |   Length      | Text  in UTF-8                |
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! Item2  |     Type      |   Length      | Text  in UTF-8                |
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//!        :                               ...                             :
//!        +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! @endcode
ROC_ATTR_PACKED_BEGIN class SdesPacket {
private:
    PacketHeader header_;

public:
    SdesPacket() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(RTCP_SDES);
    }

    //! Get common packet header.
    const PacketHeader& header() const {
        return header_;
    }

    //! Get common packet header.
    PacketHeader& header() {
        return header_;
    }
} ROC_ATTR_PACKED_END;

//! BYE source header.
//!
//! Part of BYE packet.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |                              SSRC                             |
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! @endcode
ROC_ATTR_PACKED_BEGIN class ByeSourceHeader {
private:
    uint32_t ssrc_;

public:
    ByeSourceHeader() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        ssrc_ = 0;
    }

    //! Get SSRC.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC.
    void set_ssrc(const packet::stream_source_t s) {
        ssrc_ = core::hton32u(s);
    }
} ROC_ATTR_PACKED_END;

//! BYE reason header.
//!
//! Part of BYE packet.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |     length    |               reason for leaving            ...
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! @endcode
ROC_ATTR_PACKED_BEGIN class ByeReasonHeader {
private:
    uint8_t len_;

public:
    //! Get maximum allowed reason text length.
    static const size_t MaxTextLen = 255;

    ByeReasonHeader() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        len_ = 0;
    }

    //! Get text length.
    size_t text_len() const {
        return len_;
    }

    //! Set text length.
    void set_text_len(const size_t len) {
        roc_panic_if(len > MaxTextLen);
        len_ = (uint8_t)len;
    }

    //! Get pointer to text.
    //! The text is NOT zero-terminated.
    const uint8_t* text() const {
        return (const uint8_t*)this + sizeof(*this);
    }

    //! Get pointer to text.
    //! The text is NOT zero-terminated.
    uint8_t* text() {
        return (uint8_t*)this + sizeof(*this);
    }
} ROC_ATTR_PACKED_END;

//! Goodbye RTCP packet (BYE).
//!
//! RFC 3550 6.6. "BYE: Goodbye RTCP packet"
//!
//! @code
//!        0                   1                   2                   3
//!        0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//!       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!       |V=2|P|    SC   |   PT=Bye=203  |             length            |
//!       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!       |                           SSRC/CSRC                           |
//!       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!       :                              ...                              :
//!       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! (opt) |     length    |               reason for leaving            ...
//!       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class ByePacket {
private:
    PacketHeader header_;

public:
    ByePacket() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(RTCP_BYE);
    }

    //! Get common packet header.
    const PacketHeader& header() const {
        return header_;
    }

    //! Get common packet header.
    PacketHeader& header() {
        return header_;
    }
} ROC_ATTR_PACKED_END;

//! RTCP Extended Report Packet.
//!
//! RFC 3611 2. "XR Packet Format"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |V=2|P|reserved |   PT=Xr=207   |             length            |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                              SSRC                             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! :                         report blocks                         :
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrPacket {
private:
    PacketHeader header_;
    uint32_t ssrc_;

public:
    XrPacket() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(RTCP_XR);
        ssrc_ = 0;
    }

    //! Get common packet header.
    const PacketHeader& header() const {
        return header_;
    }

    //! Get common packet header.
    PacketHeader& header() {
        return header_;
    }

    //! Get SSRC of packet originator.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC of packet originator.
    void set_ssrc(const packet::stream_source_t ssrc) {
        ssrc_ = core::hton32u(ssrc);
    }
} ROC_ATTR_PACKED_END;

//! XR Block Type.
enum XrBlockType {
    XR_LOSS_RLE = 1,          //!< Loss RLE Report Block.
    XR_DUPLICATE_RLE = 2,     //!< Duplicate RLE Report Block.
    XR_PACKET_RECPT_TIME = 3, //!< Packet Receipt Times Report Block.
    XR_RRTR = 4,              //!< Receiver Reference Time Report Block.
    XR_DLRR = 5,              //!< DLRR Report Block.
    XR_STAT_SUMMARY = 6,      //!< Statistics Summary Report Block.
    XR_VOIP_METRICS = 7       //!< VoIP Metrics Report Block.
};

//! XR Block Header.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |      BT       | type-specific |         block length          |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! :             type-specific block contents                      :
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrBlockHeader {
private:
    uint8_t block_type_;
    uint8_t type_specific_;
    uint16_t length_;

public:
    XrBlockHeader() {
        reset(XrBlockType(0));
    }

    //! Reset to initial state (all zeros).
    void reset(const XrBlockType bt) {
        block_type_ = type_specific_ = 0;
        length_ = 0;
        set_block_type(bt);
    }

    //! Get XR block type.
    XrBlockType block_type() const {
        return (XrBlockType)block_type_;
    }

    //! Set XR block type.
    void set_block_type(const XrBlockType bt) {
        roc_panic_if_not(bt == 0 || (bt >= XR_LOSS_RLE && bt <= XR_VOIP_METRICS));
        block_type_ = (uint8_t)bt;
    }

    //! Get type-specific byte.
    uint8_t type_specific() const {
        return type_specific_;
    }

    //! Set type-specific byte.
    void set_type_specific(const uint8_t t) {
        type_specific_ = t;
    }

    //! Get block length, including the header, in 32-bit words minus one.
    uint16_t len_words() const {
        return core::ntoh16u(length_);
    }

    //! Set block length in words.
    void set_len_words(const uint16_t len) {
        length_ = core::hton16u(len);
    }

    //! Get block length, including the header, in bytes.
    size_t len_bytes() const {
        return rtcp_length_2_size_t(len_words());
    }

    //! Set block length in bytes.
    void set_len_bytes(const size_t len) {
        set_len_words(size_t_2_rtcp_length(len));
    }
} ROC_ATTR_PACKED_END;

//! XR Receiver Reference Time Report block.
//!
//! RFC 3611 4.4. "Receiver Reference Time Report Block"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |     BT=4      |   reserved    |       block length = 2        |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |              NTP timestamp, most significant word             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |             NTP timestamp, least significant word             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrRrtrBlock {
private:
    XrBlockHeader header_;
    NtpTimestamp ntp_;

public:
    XrRrtrBlock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(XR_RRTR);
        ntp_.reset();
    }

    //! Get common block header.
    const XrBlockHeader& header() const {
        return header_;
    }

    //! Get common block header.
    XrBlockHeader& header() {
        return header_;
    }

    //! Get NTP timestamp.
    packet::ntp_timestamp_t ntp_timestamp() const {
        return ntp_.value();
    }

    //! Set NTP timestamp.
    void set_ntp_timestamp(const packet::ntp_timestamp_t t) {
        ntp_.set_value(t);
    }
} ROC_ATTR_PACKED_END;

//! XR DLRR Report sub-block.
//!
//! RFC 3611 4.5. "DLRR Report Sub-block"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |                             SSRC                              |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                         last RR (LRR)                         |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                   delay since last RR (DLRR)                  |
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrDlrrSubblock {
private:
    uint32_t ssrc_;
    uint32_t last_rr_;
    uint32_t delay_last_rr_;

public:
    XrDlrrSubblock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        ssrc_ = last_rr_ = delay_last_rr_ = 0;
    }

    //! Get SSRC of receiver.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC of receiver.
    void set_ssrc(const packet::stream_source_t ssrc) {
        ssrc_ = core::hton32u(ssrc);
    }

    //! Get LRR.
    packet::ntp_timestamp_t last_rr() const {
        packet::ntp_timestamp_t x = core::ntoh32u(last_rr_);
        x <<= 16;
        return x;
    }

    //! Set LRR.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_last_rr(packet::ntp_timestamp_t x) {
        x >>= 16;
        x &= 0xffffffff;
        last_rr_ = core::hton32u((uint32_t)x);
    }

    //! Get DLRR.
    packet::ntp_timestamp_t delay_last_rr() const {
        return core::ntoh32u(delay_last_rr_);
    }

    //! Set DLRR.
    //! Stores only the low 32 bits out of 64 in the NTP timestamp.
    void set_delay_last_rr(packet::ntp_timestamp_t x) {
        x &= 0xffffffff;
        delay_last_rr_ = core::hton32u((uint32_t)x);
    }
} ROC_ATTR_PACKED_END;

//! XR DLRR Report block.
//!
//! RFC 3611 4.5. "DLRR Report Block"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |     BT=5      |   reserved    |         block length          |
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |                 SSRC_1 (SSRC of first receiver)               | sub-
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
//! |                         last RR (LRR)                         |   1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                   delay since last RR (DLRR)                  |
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! |                 SSRC_2 (SSRC of second receiver)              | sub-
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+ block
//! :                               ...                             :   2
//! +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrDlrrBlock {
private:
    XrBlockHeader header_;

public:
    XrDlrrBlock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(XR_DLRR);
    }

    //! Get common block header.
    const XrBlockHeader& header() const {
        return header_;
    }

    //! Get common block header.
    XrBlockHeader& header() {
        return header_;
    }

    //! Get number of sub-blocks.
    size_t num_subblocks() const {
        return (header_.len_bytes() - sizeof(header_)) / sizeof(XrDlrrSubblock);
    }

    //! Get DLRR sub-block by index.
    const XrDlrrSubblock& get_subblock(const size_t i) const {
        return get_block_by_index<const XrDlrrSubblock>(this, i, num_subblocks(),
                                                        "rtcp xr_dlrr");
    }

    //! Get DLRR sub-block by index.
    XrDlrrSubblock& get_subblock(const size_t i) {
        return get_block_by_index<XrDlrrSubblock>(this, i, num_subblocks(),
                                                  "rtcp xr_dlrr");
    }
} ROC_ATTR_PACKED_END;

} // namespace header
} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_HEADERS_H_
