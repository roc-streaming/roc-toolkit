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
#include "roc_core/time.h"
#include "roc_packet/ntp.h"
#include "roc_packet/units.h"

namespace roc {
namespace rtcp {
namespace header {

//! Get bits from v0.
//! @param v0 Where to read the bits.
//! @param shift From which bit num the field start.
//! @param mask The bitmask.
template <typename T> T get_bit_field(T v0, const size_t shift, const size_t mask) {
    v0 >>= shift;
    v0 &= mask;
    return v0;
}

//! Set bits in v0.
//! @param v0 Where to write the bits.
//! @param v1 The bits to write.
//! @param shift From which bit num the field start.
//! @param mask The bitmask.
template <typename T>
void set_bit_field(T& v0, const T v1, const size_t shift, const size_t mask) {
    v0 &= T(~(mask << shift));
    v0 |= T((v1 & mask) << shift);
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

//! Clamp 64-bit NTP timestamp so that it does not exceed maximum.
inline packet::ntp_timestamp_t clamp_ntp_64(packet::ntp_timestamp_t value,
                                            packet::ntp_timestamp_t max_value) {
    if (value > max_value) {
        value = max_value;
    }
    return value;
}

//! Clamp 64-bit NTP timestamp so that it fits into middle 32-bits.
//! Value is rounded to the new resolution and capped with given maximum.
//! Returned value has zeros in high and low 16 bits.
inline packet::ntp_timestamp_t clamp_ntp_32(packet::ntp_timestamp_t value,
                                            packet::ntp_timestamp_t max_value) {
    if (value & 0x8000) {
        value += 0x8000;
    }
    if (value > max_value) {
        value = max_value;
    }
    value &= 0x0000FFFFFFFF0000;
    return value;
}

//! Restore full 64-bit NTP timestamp from middle 32 bits.
//! @param value is middle 32 bits of timestamp to be restored.
//! @param base is full 64 bit timestamp that was recently obtained from same source.
//! The function will combine high 16 bits of base with value.
//! It will also detect possible wrap and apply correction if needed.
inline packet::ntp_timestamp_t extend_timestamp(packet::ntp_timestamp_t base,
                                                packet::ntp_timestamp_t value) {
    roc_panic_if_msg(value & 0xFFFF00000000FFFF, "value should have only middle 32 bits");

    // value extended with high 16 bits from base
    const packet::ntp_timestamp_t extended_value = (base & 0xFFFF000000000000) | value;
    // another candidate: same, but assuming that it wrapped arround before
    // truncating high 16 bits
    const packet::ntp_timestamp_t wrapped_value = extended_value + 0x0001000000000000;

    // choose candidate that is closer to base
    if (std::abs(int64_t(extended_value - base))
        <= std::abs(int64_t(wrapped_value - base))) {
        return extended_value;
    }
    return wrapped_value;
}

//! Maximum number of inner blocks/chunks in RTCP packet.
static const size_t MaxPacketBlocks = 31;

//! Maximum allowed SDES/BYE text length.
static const size_t MaxTextLen = 255;

//! Maximum allowed DLSR/DLRR value.
static const packet::ntp_timestamp_t MaxDelay = 0x0000FFFFFFFFFFFF;

//! Special value when metric is not available (64-bit).
static const packet::ntp_timestamp_t MetricUnavail_64 = 0xFFFFFFFFFFFFFFFF;

//! Special value when metric is not available (32-bit).
static const packet::ntp_timestamp_t MetricUnavail_32 = 0x0000FFFFFFFF0000;

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

//! RTCP packet header, common for all RTCP packet types.
//!
//! RFC 3550 6.4.1: "SR: Sender Report RTCP Packet"
//! RFC 3550 6.4.2: "RR: Receiver Report RTCP Packet"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |V=2|P|    RC   |       PT      |             length            |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class PacketHeader {
private:
    enum {
        //! @name RTCP protocol version.
        // @{
        Version_shift = 6,
        Version_mask = 0x03,
        // @}

        //! @name RTCP padding flag.
        // @{
        Padding_shift = 5,
        Padding_mask = 0x01,
        // @}

        //! @name RTCP packets counter.
        // @{
        Counter_shift = 0,
        Counter_mask = 0x1F
        // @}
    };

    // Protocol version, padding flag, and block/chunk counter.
    // Varies by packet type.
    uint8_t count_;
    // RTCP packet type.
    uint8_t type_;
    // Packet length in 4-byte words, w/o common packet header word.
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
        return get_bit_field<uint8_t>(count_, Counter_shift, Counter_mask);
    }

    //! Set number of blocks/chunks.
    void set_counter(const size_t c) {
        roc_panic_if(c > MaxPacketBlocks);
        set_bit_field<uint8_t>(count_, (uint8_t)c, Counter_shift, Counter_mask);
    }

    //! Increment packet counter,
    void inc_counter() {
        return set_counter(counter() + 1);
    }

    //! Get protocol version.
    uint8_t version() const {
        return get_bit_field<uint8_t>(count_, Version_shift, Version_mask);
    }

    //! Set protocol version.
    void set_version(const Version v) {
        roc_panic_if((v & Version_mask) != v);
        set_bit_field<uint8_t>(count_, v, Version_shift, Version_mask);
    }

    //! Get padding flag.
    bool has_padding() const {
        return get_bit_field(count_, Padding_shift, Padding_mask);
    }

    //! Set padding flag.
    void set_padding(const bool v) {
        set_bit_field(count_, (uint8_t)v, Padding_shift, Padding_mask);
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

//! 64-bit NTP timestamp.
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |              NTP timestamp, most significant word             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |             NTP timestamp, least significant word             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
//!
//! From RFC 3550.
ROC_ATTR_PACKED_BEGIN class NtpTimestamp64 {
private:
    uint32_t high_;
    uint32_t low_;

public:
    NtpTimestamp64() {
        set_value(0);
    }

    //! Get NTP timestamp value.
    packet::ntp_timestamp_t value() const {
        return ((packet::ntp_timestamp_t)core::ntoh32u(high_) << 32)
            | (packet::ntp_timestamp_t)core::ntoh32u(low_);
    }

    //! Set NTP timestamp value.
    void set_value(const packet::ntp_timestamp_t t) {
        high_ = core::hton32u((uint32_t)(t >> 32));
        low_ = core::hton32u((uint32_t)t);
    }
} ROC_ATTR_PACKED_END;

//! 32-bit NTP absolute time (stored as middle 32 bits of 64-bit timestamp).
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                              Time                             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
//!
//! From RFC 3550.
ROC_ATTR_PACKED_BEGIN class NtpTimestamp32 {
private:
    uint32_t mid_;

public:
    NtpTimestamp32() {
        set_value(0);
    }

    //! Get NTP timestamp value.
    packet::ntp_timestamp_t value() const {
        return (packet::ntp_timestamp_t)core::ntoh32u(mid_) << 16;
    }

    //! Set NTP timestamp value.
    //! Stores middle 32 bits of timestamp.
    //! High and low 16 bits are just truncated.
    void set_value(const packet::ntp_timestamp_t t) {
        mid_ = core::hton32u(t >> 16);
    }
} ROC_ATTR_PACKED_END;

//! Reception report block.
//!
//! Part of RR and SR packets.
//!
//! RFC 3550 6.4.1: "SR: Sender Report RTCP Packet"
//! RFC 3550 6.4.2: "RR: Receiver Report RTCP Packet"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
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
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//!  @endcode
ROC_ATTR_PACKED_BEGIN class ReceptionReportBlock {
private:
    enum {
        //! @name Fraction lost since last SR/RR.
        // @{
        FractLost_shift = 24,
        FractLoss_width = 8,
        FractLost_mask = 0xFF,
        // @}

        //! @name Cumulative number of packets lost since the beginning.
        // @{
        CumLoss_shift = 0,
        CumLoss_width = 24,
        CumLoss_mask = 0xFFFFFF
        // @}
    };

    // Data source being reported.
    uint32_t ssrc_;
    // Fraction lost since last SR/RR and cumulative number of
    // packets lost since the beginning of reception (signed!).
    uint32_t losses_;
    // Extended last seq. no. received.
    uint32_t last_seq_;
    // Interarrival jitter.
    uint32_t jitter_;
    // Last SR packet from this source.
    NtpTimestamp32 last_sr_;
    // Delay since last SR packet.
    NtpTimestamp32 delay_last_sr_;

public:
    ReceptionReportBlock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        ssrc_ = losses_ = last_seq_ = jitter_ = 0;
        last_sr_.set_value(0);
        delay_last_sr_.set_value(0);
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
            get_bit_field<uint32_t>(losses, FractLost_shift, FractLost_mask);

        return float(fract_loss8) / float(1 << FractLoss_width);
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
            (uint8_t)(uint32_t)(fract_loss * float(1 << FractLoss_width));

        uint32_t losses = core::ntoh32u(losses_);
        set_bit_field<uint32_t>(losses, fract_loss8, FractLost_shift, FractLost_mask);

        losses_ = core::hton32u(losses);
    }

    //! Get cumulative loss.
    //! May be negative in case of packet duplications.
    long cum_loss() const {
        const uint32_t losses = core::ntoh32u(losses_);

        uint32_t cum_loss = get_bit_field<uint32_t>(losses, CumLoss_shift, CumLoss_mask);

        // If cum_loss is negative
        if (cum_loss & (1 << (CumLoss_width - 1))) {
            // Make whole leftest byte filled with 1.
            cum_loss |= ~(uint32_t)CumLoss_mask;
        }

        return (long)(int32_t)cum_loss;
    }

    //! Set cumulative loss.
    //! May be negative in case of packet duplications.
    void set_cum_loss(long cum_loss) {
        if (cum_loss > CumLoss_mask) {
            cum_loss = CumLoss_mask;
        } else if (cum_loss < -(int32_t)CumLoss_mask) {
            cum_loss = -CumLoss_mask;
        }

        uint32_t losses = core::ntoh32u(losses_);
        set_bit_field<uint32_t>(losses, (uint32_t)(int32_t)cum_loss, CumLoss_shift,
                                CumLoss_mask);

        losses_ = core::hton32u(losses);
    }

    //! Get last seqnum.
    packet::ext_seqnum_t last_seqnum() const {
        return core::ntoh32u(last_seq_);
    }

    //! Set last seqnum.
    void set_last_seqnum(const packet::ext_seqnum_t x) {
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
        return last_sr_.value();
    }

    //! Set LSR.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_last_sr(const packet::ntp_timestamp_t x) {
        last_sr_.set_value(x);
    }

    //! Get DLSR.
    packet::ntp_timestamp_t delay_last_sr() const {
        return delay_last_sr_.value();
    }

    //! Set DLSR.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_delay_last_sr(const packet::ntp_timestamp_t x) {
        delay_last_sr_.set_value(clamp_ntp_32(x, MaxDelay));
    }
} ROC_ATTR_PACKED_END;

//! Receiver Report RTCP packet (RR).
//!
//! RFC 3550 6.4.2: "RR: Receiver Report RTCP packet"
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

    // Data source being reported.
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
    NtpTimestamp64 ntp_timestamp_;
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
        ntp_timestamp_.set_value(0);
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
    // RFC 3550
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
//! RFC 3550 6.5: "SDES: Source Description RTCP packet"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                              SSRC                             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
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
//! RFC 3550 6.5: "SDES: Source Description RTCP packet"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |     Type      |   Length      | Text  in UTF-8              ...
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class SdesItemHeader {
private:
    uint8_t type_;
    uint8_t len_;

public:
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
//! RFC 3550 6.5: "SDES: Source Description RTCP packet"
//!
//! @code
//!         0                   1                   2                   3
//!         0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//!        +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! header |V=2|P|    RC   |   PT=SDES=202  |             length           |
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
//! RFC 3550 6.6: "BYE: Goodbye RTCP Packet"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                              SSRC                             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
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
//! RFC 3550 6.6: "BYE: Goodbye RTCP Packet"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |     length    |               reason for leaving            ...
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class ByeReasonHeader {
private:
    uint8_t len_;

public:
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
//!       |V=2|P|    SC   |   PT=BYE=203  |             length            |
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
//! RFC 3611 2: "XR Packet Format"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |V=2|P|reserved |   PT=XR=207   |             length            |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                              SSRC                             |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! :                         report blocks                         :
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrPacket {
private:
    PacketHeader header_;

    // Data source being reported.
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
    // RFC 3611
    XR_RRTR = 4, //!< RRTR Report Block.
    XR_DLRR = 5, //!< DLRR Report Block.
    // RFC 6776
    XR_MEASUREMENT_INFO = 14, //!< Measurement Information Report Block.
    // RFC 6843
    XR_DELAY_METRICS = 16, //!< Delay Metrics Report Block.
    // Non-standard
    XR_QUEUE_METRICS = 220 //!< Queue Metrics Report Block.
};

//! XR Block Header.
//!
//! RFC 3611 3: "Extended Report Block Framework"
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
//! RFC 3611 4.4: "Receiver Reference Time Report Block"
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

    // Report time.
    NtpTimestamp64 ntp_timestamp_;

public:
    XrRrtrBlock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(XR_RRTR);
        ntp_timestamp_.set_value(0);
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
        return ntp_timestamp_.value();
    }

    //! Set NTP timestamp.
    void set_ntp_timestamp(const packet::ntp_timestamp_t t) {
        ntp_timestamp_.set_value(t);
    }
} ROC_ATTR_PACKED_END;

//! XR DLRR Report sub-block.
//!
//! RFC 3611 4.5: "DLRR Report Sub-block"
//!
//! @code
//!  0                   1                   2                   3
//!  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                             SSRC                              |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                         last RR (LRR)                         |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                   delay since last RR (DLRR)                  |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrDlrrSubblock {
private:
    uint32_t ssrc_;
    NtpTimestamp32 last_rr_;
    NtpTimestamp32 delay_last_rr_;

public:
    XrDlrrSubblock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        ssrc_ = 0;
        last_rr_.set_value(0);
        delay_last_rr_.set_value(0);
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
        return last_rr_.value();
    }

    //! Set LRR.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_last_rr(const packet::ntp_timestamp_t x) {
        last_rr_.set_value(x);
    }

    //! Get DLRR.
    packet::ntp_timestamp_t delay_last_rr() const {
        return delay_last_rr_.value();
    }

    //! Set DLRR.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_delay_last_rr(const packet::ntp_timestamp_t x) {
        delay_last_rr_.set_value(clamp_ntp_32(x, MaxDelay));
    }
} ROC_ATTR_PACKED_END;

//! XR DLRR Report block.
//!
//! Provides delay since last receiver report (DLRR) for each receiver,
//! complementing to DLSR.
//!
//! RFC 3611 4.5: "DLRR Report Block"
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

//! XR Measurement Info Report Block.
//!
//! Defines measurement interval associated with other metrics blocks,
//! in particular XrDelayMetricsBlock.
//!
//! RFC 6776 4.1: "Report Block Structure"
//!
//! @code
//!  0               1               2               3
//!  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |     BT=14     |    Reserved   |      block length = 7         |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                    SSRC of stream source                      |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |            Reserved           |    first sequence number      |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |           extended first sequence number of interval          |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                 extended last sequence number                 |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |              Measurement Duration (Interval)                  |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |     Measurement Duration (Cumulative) - Seconds (bit 0-31)    |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |     Measurement Duration (Cumulative) - Fraction (bit 0-31)   |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrMeasurementInfoBlock {
private:
    XrBlockHeader header_;

    uint32_t ssrc_;
    uint16_t reserved_;
    uint16_t first_seq_;
    uint32_t interval_first_seq_;
    uint32_t interval_last_seq_;
    NtpTimestamp32 interval_duration_;
    NtpTimestamp64 cum_duration_;

public:
    XrMeasurementInfoBlock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(XR_MEASUREMENT_INFO);
        ssrc_ = 0;
        reserved_ = 0;
        first_seq_ = 0;
        interval_first_seq_ = interval_last_seq_ = 0;
        interval_duration_.set_value(0);
        cum_duration_.set_value(0);
    }

    //! Get common block header.
    const XrBlockHeader& header() const {
        return header_;
    }

    //! Get common block header.
    XrBlockHeader& header() {
        return header_;
    }

    //! Get SSRC of source being reported.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC of source being reported.
    void set_ssrc(const packet::stream_source_t ssrc) {
        ssrc_ = core::hton32u(ssrc);
    }

    //! Get seqnum of first ever received packet.
    packet::seqnum_t first_seq() const {
        return core::ntoh16u(first_seq_);
    }

    //! Set seqnum of first ever received packet.
    void set_first_seq(const packet::seqnum_t x) {
        first_seq_ = core::hton16u(x);
    }

    //! Get extended seqnum of first packet in interval.
    packet::ext_seqnum_t interval_first_seq() const {
        return core::ntoh32u(interval_first_seq_);
    }

    //! Set extended seqnum of first packet in interval.
    void set_interval_first_seq(const packet::ext_seqnum_t x) {
        interval_first_seq_ = core::hton32u(x);
    }

    //! Get extended seqnum of last packet in interval.
    packet::ext_seqnum_t interval_last_seq() const {
        return core::ntoh32u(interval_last_seq_);
    }

    //! Set extended seqnum of last packet in interval.
    void set_interval_last_seq(const packet::ext_seqnum_t x) {
        interval_last_seq_ = core::hton32u(x);
    }

    //! Get measurement interval duration.
    //! Applicable to MetricFlag_IntervalDuration reports.
    packet::ntp_timestamp_t interval_duration() const {
        return interval_duration_.value();
    }

    //! Set measurement interval duration.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_interval_duration(const packet::ntp_timestamp_t x) {
        interval_duration_.set_value(clamp_ntp_32(x, MaxDelay));
    }

    //! Get measurement cumulative duration.
    //! Applicable to MetricFlag_CumulativeDuration reports.
    packet::ntp_timestamp_t cum_duration() const {
        return cum_duration_.value();
    }

    //! Set measurement cumulative duration.
    void set_cum_duration(const packet::ntp_timestamp_t t) {
        cum_duration_.set_value(t);
    }
};

//! Interval Metric flag for XR Delay Metrics Block.
enum MetricFlag {
    //! Interval Duration.
    //! The reported value applies to the most recent measurement interval
    //! duration between successive metrics reports.
    MetricFlag_IntervalDuration = 0x2,

    //! Cumulative Duration.
    //! The reported value applies to the accumulation period characteristic
    //! of cumulative measurements.
    MetricFlag_CumulativeDuration = 0x3,

    //! Sampled Value.
    //! The reported value is a sampled instantaneous value.
    MetricFlag_SampledValue = 0x1
};

//! XR Delay Metrics Block.
//!
//! RFC 6843 3.1: "Report Block Structure"
//!
//! @code
//!  0               1               2               3
//!  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |    BT=16      | I |   resv.   |      block length = 6         |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                           SSRC of Source                      |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                  Mean Network Round-Trip Delay                |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                   Min Network Round-Trip Delay                |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                   Max Network Round-Trip Delay                |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |               End System Delay - Seconds (bit 0-31)           |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |              End System Delay - Fraction (bit 0-31)           |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrDelayMetricsBlock {
private:
    enum {
        MetricFlag_shift = 6,
        MetricFlag_mask = 0x03,
    };

    XrBlockHeader header_;

    uint32_t ssrc_;
    NtpTimestamp32 mean_rtt_;
    NtpTimestamp32 min_rtt_;
    NtpTimestamp32 max_rtt_;
    NtpTimestamp64 e2e_delay_;

public:
    XrDelayMetricsBlock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(XR_DELAY_METRICS);
        ssrc_ = 0;
        mean_rtt_.set_value(MetricUnavail_32);
        min_rtt_.set_value(MetricUnavail_32);
        max_rtt_.set_value(MetricUnavail_32);
        e2e_delay_.set_value(MetricUnavail_64);
    }

    //! Get common block header.
    const XrBlockHeader& header() const {
        return header_;
    }

    //! Get common block header.
    XrBlockHeader& header() {
        return header_;
    }

    //! Get Interval Metrics flag.
    MetricFlag metric_flag() const {
        return (MetricFlag)get_bit_field<uint8_t>(header_.type_specific(),
                                                  MetricFlag_shift, MetricFlag_mask);
    }

    //! Set Interval Metrics flag.
    void set_metric_flag(const MetricFlag f) {
        uint8_t t = header_.type_specific();
        set_bit_field<uint8_t>(t, (uint8_t)f, MetricFlag_shift, MetricFlag_mask);
        header_.set_type_specific(t);
    }

    //! Get SSRC of source being reported.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC of source being reported.
    void set_ssrc(const packet::stream_source_t ssrc) {
        ssrc_ = core::hton32u(ssrc);
    }

    //! Check if Mean Network Round-Trip Delay is set.
    bool has_mean_rtt() const {
        return mean_rtt_.value() != MetricUnavail_32;
    }

    //! Get Mean Network Round-Trip Delay.
    packet::ntp_timestamp_t mean_rtt() const {
        return mean_rtt_.value();
    }

    //! Set Mean Network Round-Trip Delay.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_mean_rtt(const packet::ntp_timestamp_t x) {
        mean_rtt_.set_value(clamp_ntp_32(x, MetricUnavail_32 - 1));
    }

    //! Check if Minimum Network Round-Trip Delay is set.
    bool has_min_rtt() const {
        return min_rtt_.value() != MetricUnavail_32;
    }

    //! Get Minimum Network Round-Trip Delay.
    packet::ntp_timestamp_t min_rtt() const {
        return min_rtt_.value();
    }

    //! Set Minimum Network Round-Trip Delay.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_min_rtt(const packet::ntp_timestamp_t x) {
        min_rtt_.set_value(clamp_ntp_32(x, MetricUnavail_32 - 1));
    }

    //! Check if Maximum Network Round-Trip Delay is set.
    bool has_max_rtt() const {
        return max_rtt_.value() != MetricUnavail_32;
    }

    //! Get Maximum Network Round-Trip Delay.
    packet::ntp_timestamp_t max_rtt() const {
        return max_rtt_.value();
    }

    //! Set Maximum Network Round-Trip Delay.
    //! Stores only the middle 32 bits out of 64 in the NTP timestamp.
    void set_max_rtt(const packet::ntp_timestamp_t x) {
        max_rtt_.set_value(clamp_ntp_32(x, MetricUnavail_32 - 1));
    }

    //! Check if End System Delay is set.
    bool has_e2e_delay() const {
        return e2e_delay_.value() != MetricUnavail_64;
    }

    //! Get End System Delay.
    packet::ntp_timestamp_t e2e_delay() const {
        return e2e_delay_.value();
    }

    //! Set End System Delay.
    void set_e2e_delay(const packet::ntp_timestamp_t t) {
        e2e_delay_.set_value(clamp_ntp_64(t, MetricUnavail_64 - 1));
    }
} ROC_ATTR_PACKED_END;

//! XR Queue Metrics Block.
//!
//! Non-standard.
//!
//! @code
//!  0               1               2               3
//!  0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |    BT=220     | I |   resv.   |      block length = 6         |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                           SSRC of Source                      |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! |                    Network Incoming Queue Delay               |
//! +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//! @endcode
ROC_ATTR_PACKED_BEGIN class XrQueueMetricsBlock {
private:
    enum {
        MetricFlag_shift = 6,
        MetricFlag_mask = 0x03,
    };

    XrBlockHeader header_;

    uint32_t ssrc_;
    NtpTimestamp32 niq_delay_;

public:
    XrQueueMetricsBlock() {
        reset();
    }

    //! Reset to initial state (all zeros).
    void reset() {
        header_.reset(XR_QUEUE_METRICS);
        ssrc_ = 0;
        niq_delay_.set_value(MetricUnavail_32);
    }

    //! Get common block header.
    const XrBlockHeader& header() const {
        return header_;
    }

    //! Get common block header.
    XrBlockHeader& header() {
        return header_;
    }

    //! Get Interval Metrics flag.
    MetricFlag metric_flag() const {
        return (MetricFlag)get_bit_field<uint8_t>(header_.type_specific(),
                                                  MetricFlag_shift, MetricFlag_mask);
    }

    //! Set Interval Metrics flag.
    void set_metric_flag(const MetricFlag f) {
        uint8_t t = header_.type_specific();
        set_bit_field<uint8_t>(t, (uint8_t)f, MetricFlag_shift, MetricFlag_mask);
        header_.set_type_specific(t);
    }

    //! Get SSRC of source being reported.
    packet::stream_source_t ssrc() const {
        return core::ntoh32u(ssrc_);
    }

    //! Set SSRC of source being reported.
    void set_ssrc(const packet::stream_source_t ssrc) {
        ssrc_ = core::hton32u(ssrc);
    }

    //! Check if Network Incoming Queue Delay is set.
    bool has_niq_delay() const {
        return niq_delay_.value() != MetricUnavail_32;
    }

    //! Get Network Incoming Queue Delay.
    packet::ntp_timestamp_t niq_delay() const {
        return niq_delay_.value();
    }

    //! Set Network Incoming Queue Delay.
    void set_niq_delay(const packet::ntp_timestamp_t t) {
        niq_delay_.set_value(clamp_ntp_32(t, MetricUnavail_32 - 1));
    }
} ROC_ATTR_PACKED_END;

} // namespace header
} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_HEADERS_H_
