/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/fec_packet.h
//! @brief RTP FEC packet.

#ifndef ROC_RTP_FEC_PACKET_H_
#define ROC_RTP_FEC_PACKET_H_

#include "roc_core/noncopyable.h"
#include "roc_core/ipool.h"
#include "roc_packet/ifec_packet.h"
#include "roc_rtp/rtp_packet.h"

namespace roc {
namespace rtp {

//! RTP FEC packet.
class FECPacket : public packet::IFECPacket, public core::NonCopyable<> {
public:
    //! Initialize.
    FECPacket(core::IPool<FECPacket>&, const RTP_Packet&);

    //! Get packet source ID.
    virtual packet::source_t source() const;

    //! Set packet source ID.
    virtual void set_source(packet::source_t);

    //! Get packet sequence number.
    virtual packet::seqnum_t seqnum() const;

    //! Set packet sequence number.
    virtual void set_seqnum(packet::seqnum_t);

    //! Get packet timestamp.
    virtual packet::timestamp_t timestamp() const;

    //! Set packet timestamp.
    virtual void set_timestamp(packet::timestamp_t);

    //! Get packet marker bit.
    virtual bool marker() const;

    //! Set packet marker bit.
    virtual void set_marker(bool);

    //! Seqnum of first data packet in block.
    virtual packet::seqnum_t data_blknum() const;

    //! Set seqnum of first data packet in block.
    virtual void set_data_blknum(packet::seqnum_t);

    //! Seqnum of first FEC packet in block.
    virtual packet::seqnum_t fec_blknum() const;

    //! Set seqnum of first FEC packet in block.
    virtual void set_fec_blknum(packet::seqnum_t);

    //! Get payload.
    virtual core::IByteBufferConstSlice payload() const;

    //! Set payload data and size.
    virtual void set_payload(const uint8_t* data, size_t size);

    //! Get packet data buffer (containing header and payload).
    virtual core::IByteBufferConstSlice raw_data() const;

private:
    virtual void free();

    RTP_Packet packet_;
    core::IPool<FECPacket>& pool_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_FEC_PACKET_H_
