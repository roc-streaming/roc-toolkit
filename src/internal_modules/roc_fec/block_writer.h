/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/block_writer.h
//! @brief FEC writer for block codes.

#ifndef ROC_FEC_BLOCK_WRITER_H_
#define ROC_FEC_BLOCK_WRITER_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_factory.h"

namespace roc {
namespace fec {

//! FEC writer parameters.
struct BlockWriterConfig {
    //! Number of data packets in block.
    size_t n_source_packets;

    //! Number of FEC packets in block.
    size_t n_repair_packets;

    BlockWriterConfig()
        : n_source_packets(18)
        , n_repair_packets(10) {
    }
};

//! FEC writer for block codes.
//! Works on top of fec::IBlockEncoder, which performs codec-specific operations.
//! @remarks
//!  You write audio packets to fec::BlockWriter.
//!  fec::BlockWriter produces to interleaved streams:
//!   - stream of source packets - original media packets + FEC meta-data
//!   - stream of repair packets - packets with redundancy
//!  Interleaved stream of source + repair packets is written to output writer.
class BlockWriter : public packet::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    BlockWriter(const BlockWriterConfig& config,
                packet::FecScheme fec_scheme,
                IBlockEncoder& block_encoder,
                packet::IWriter& writer,
                packet::IComposer& source_composer,
                packet::IComposer& repair_composer,
                packet::PacketFactory& packet_factory,
                core::IArena& arena);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Get maximal FEC block duration seen since last block resize.
    packet::stream_timestamp_t max_block_duration() const;

    //! Set number of source packets per block.
    //! @note
    //!  Actual reallocation may happen later.
    ROC_ATTR_NODISCARD status::StatusCode resize(size_t sblen, size_t rblen);

    //! Write packet.
    //! @remarks
    //!  - writes the given source packet to the output writer
    //!  - generates repair packets and also writes them to the output writer
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const packet::PacketPtr&);

private:
    status::StatusCode begin_block_(const packet::PacketPtr& pp);
    status::StatusCode end_block_();
    void next_block_();

    bool apply_sizes_(size_t sblen, size_t rblen, size_t payload_size);

    status::StatusCode write_source_packet_(const packet::PacketPtr&);
    status::StatusCode make_repair_packets_();
    status::StatusCode make_repair_packet_(packet::seqnum_t n, packet::PacketPtr& pp);
    status::StatusCode encode_repair_packets_();
    status::StatusCode compose_repair_packets_();
    status::StatusCode write_repair_packets_();
    void fill_packet_fec_fields_(const packet::PacketPtr& packet, packet::seqnum_t n);

    void validate_packet_(const packet::PacketPtr&);

    void update_block_duration_(const packet::PacketPtr& curr_block_pkt);

    size_t cur_sblen_;
    size_t next_sblen_;

    size_t cur_rblen_;
    size_t next_rblen_;

    size_t cur_payload_size_;

    IBlockEncoder& block_encoder_;
    packet::IWriter& pkt_writer_;

    packet::IComposer& source_composer_;
    packet::IComposer& repair_composer_;

    packet::PacketFactory& packet_factory_;

    core::Array<packet::PacketPtr> repair_block_;

    bool first_packet_;

    packet::blknum_t cur_sbn_;
    packet::seqnum_t cur_block_repair_sn_;

    size_t cur_packet_;

    const packet::FecScheme fec_scheme_;

    bool prev_block_timestamp_valid_;
    packet::stream_timestamp_t prev_block_timestamp_;
    packet::stream_timestamp_diff_t block_max_duration_;

    status::StatusCode init_status_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_BLOCK_WRITER_H_
