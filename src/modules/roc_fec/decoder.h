/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/decoder.h
//! @brief FEC decoder.

#ifndef ROC_FEC_DECODER_H_
#define ROC_FEC_DECODER_H_

#include "roc_config/config.h"

#include "roc_core/array.h"
#include "roc_core/byte_buffer.h"
#include "roc_core/noncopyable.h"

#include "roc_packet/ipacket.h"
#include "roc_packet/ipacket_parser.h"
#include "roc_packet/ipacket_reader.h"
#include "roc_packet/packet_queue.h"

#include "roc_fec/iblock_decoder.h"

namespace roc {
namespace fec {

//! FEC decoder.
//! @remarks
//!  Reads data and FEC packets from input queues and restores missing data packets.
class Decoder : public packet::IPacketReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p block_decoder specifies FEC codec implementation;
    //!  - @p data_reader specifies input queue with data packets;
    //!  - @p fec_reader specifies input queue with FEC packets;
    //!  - @p parser specifies packet parser for restored packets.
    Decoder(IBlockDecoder& block_decoder,
            packet::IPacketReader& data_reader,
            packet::IPacketReader& fec_reader,
            packet::IPacketParser& parser);

    //! Get packet.
    //! @returns next available packet.
    //! @remarks
    //!  When packet loss is detected, also tries to restore it from FEC
    //!  packets and return repaired packet.
    virtual packet::IPacketConstPtr read();

    //! Did decoder catch block beginning?
    bool is_started() const;

    //! Is decoder alive?
    bool is_alive() const;

private:
    packet::IPacketConstPtr read_();
    packet::IPacketConstPtr get_next_packet_();

    void next_block_();
    void try_repair_();
    bool check_packet_(const packet::IPacketConstPtr&, size_t pos);

    void fetch_packets_();
    void update_packets_();

    void update_source_packets_();
    void update_repair_packets_();

    // drops outdated packets from the repair_queue_ until meets packets from the
    // current block or later
    void skip_repair_packets_();

    IBlockDecoder& block_decoder_;

    packet::IPacketReader& source_reader_;
    packet::IPacketReader& repair_reader_;
    packet::IPacketParser& parser_;

    packet::PacketQueue source_queue_;
    packet::PacketQueue repair_queue_;

    core::Array<packet::IPacketConstPtr, ROC_CONFIG_MAX_FEC_BLOCK_SOURCE_PACKETS>
        source_block_;

    core::Array<packet::IPacketConstPtr, ROC_CONFIG_MAX_FEC_BLOCK_REPAIR_PACKETS>
        repair_block_;

    bool is_alive_;
    bool is_started_;
    bool can_repair_;

    size_t next_packet_;
    packet::seqnum_t cur_block_sn_;

    bool has_source_;
    packet::source_t source_;

    unsigned n_packets_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_DECODER_H_
