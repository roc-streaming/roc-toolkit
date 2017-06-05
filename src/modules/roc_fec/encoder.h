/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/encoder.h
//! @brief FEC encoder.

#ifndef ROC_FEC_ENCODER_H_
#define ROC_FEC_ENCODER_H_

#include "roc_config/config.h"

#include "roc_core/noncopyable.h"
#include "roc_core/byte_buffer.h"
#include "roc_core/array.h"

#include "roc_packet/ipacket_writer.h"
#include "roc_packet/ipacket_composer.h"
#include "roc_packet/ipacket.h"
#include "roc_packet/ifec_packet.h"

#include "roc_fec/iblock_encoder.h"

namespace roc {
namespace fec {

//! FEC encoder.
//! @remarks
//!  Writes data packets to output queue, generates additional FEC packets and
//!  writes them to output queue too.
class Encoder : public packet::IPacketWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p block_encoder specifies FEC codec implementation;
    //!  - @p output specifies output queue for data and FEC packets;
    //!  - @p composer specifies packet composer for FEC packets.
    Encoder(IBlockEncoder& block_encoder,
            packet::IPacketWriter& output,
            packet::IPacketComposer& composer);

    //! Add data packet.
    //! @remarks
    //!  - adds data packet to output writer;
    //!  - periodically generates FEC packets and also adds them to output writer.
    virtual void write(const packet::IPacketPtr&);

private:
    //! Create FEC-packet.
    packet::IFECPacketPtr make_fec_packet_(const core::IByteBufferConstSlice& buff,
                                           const packet::seqnum_t block_data_seqnum,
                                           const packet::seqnum_t block_fec_seqnum,
                                           const packet::seqnum_t seqnum,
                                           const bool marker_bit);

    IBlockEncoder& block_encoder_;

    packet::IPacketWriter& packet_output_;
    packet::IPacketComposer& packet_composer_;

    packet::source_t source_;
    bool first_packet_;

    packet::seqnum_t cur_block_seqnum_;
    packet::seqnum_t cur_session_fec_seqnum_;

    size_t cur_data_pack_i_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_ENCODER_H_
