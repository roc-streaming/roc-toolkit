/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/block_reader.h
//! @brief FEC reader for block codes.

#ifndef ROC_FEC_BLOCK_READER_H_
#define ROC_FEC_BLOCK_READER_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_fec/iblock_decoder.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_factory.h"
#include "roc_packet/sorted_queue.h"

namespace roc {
namespace fec {

//! FEC reader parameters.
struct BlockReaderConfig {
    //! Maximum allowed source block number jump.
    size_t max_sbn_jump;

    BlockReaderConfig()
        : max_sbn_jump(100) {
    }
};

//! FEC reader for block codes.
//! Works on top of fec::IBlockDecoder, which performs codec-specific operations.
//! @remarks
//!  You read packets from fec::BlockReader.
//!  fec::BlockReader fetches packets streams from two readers:
//!   - stream of source packets - media packets + FEC meta-data
//!   - stream of repair packets - packets with redundancy
//!  If there are no losses, fec::BlockReader just returns source (media)
//!  packets and ignores repair packets.
//!  If there are losses, fec::BlockReader tries to repair missing media packets
//!  and insert them into the returned stream.
//!  Losses are detected by gaps in seqnums.
class BlockReader : public packet::IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    BlockReader(const BlockReaderConfig& config,
                packet::FecScheme fec_scheme,
                IBlockDecoder& block_decoder,
                packet::IReader& source_reader,
                packet::IReader& repair_reader,
                packet::IParser& parser,
                packet::PacketFactory& packet_factory,
                core::IArena& arena);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Did decoder catch block beginning?
    bool is_started() const;

    //! Get maximal FEC block duration seen since last block resize.
    packet::stream_timestamp_t max_block_duration() const;

    //! Read packet.
    //! @remarks
    //!  When a packet loss is detected, try to restore it from repair packets.
    virtual ROC_ATTR_NODISCARD status::StatusCode read(packet::PacketPtr& packet,
                                                       packet::PacketReadMode mode);

private:
    status::StatusCode try_start_();
    status::StatusCode get_next_packet_(packet::PacketPtr& packet,
                                        packet::PacketReadMode mode);
    status::StatusCode next_block_();

    status::StatusCode try_repair_();
    status::StatusCode parse_repaired_packet_(const core::Slice<uint8_t>& buffer,
                                              packet::PacketPtr& packet);

    status::StatusCode fetch_all_packets_();
    status::StatusCode fetch_packets_(packet::IReader&, packet::IWriter&);

    status::StatusCode fill_block_();
    status::StatusCode fill_source_block_();
    status::StatusCode fill_repair_block_();

    status::StatusCode process_source_packet_(const packet::PacketPtr&);
    status::StatusCode process_repair_packet_(const packet::PacketPtr&);

    bool validate_fec_packet_(const packet::PacketPtr&);
    bool validate_sbn_sequence_(const packet::PacketPtr&);

    bool validate_incoming_source_packet_(const packet::PacketPtr&);
    bool validate_incoming_repair_packet_(const packet::PacketPtr&);

    bool can_update_payload_size_(size_t);
    bool can_update_source_block_size_(size_t);
    bool can_update_repair_block_size_(size_t);

    status::StatusCode update_payload_size_(size_t);
    status::StatusCode update_source_block_size_(size_t);
    status::StatusCode update_repair_block_size_(size_t);

    void drop_repair_packets_from_prev_blocks_();
    void update_block_duration_(const packet::PacketPtr& curr_block_pkt);

    IBlockDecoder& block_decoder_;

    packet::IReader& source_reader_;
    packet::IReader& repair_reader_;
    packet::IParser& parser_;
    packet::PacketFactory& packet_factory_;

    packet::SortedQueue source_queue_;
    packet::SortedQueue repair_queue_;

    core::Array<packet::PacketPtr> source_block_;
    core::Array<packet::PacketPtr> repair_block_;

    bool started_;
    bool can_repair_;

    size_t head_index_;
    packet::blknum_t cur_sbn_;

    size_t payload_size_;

    bool source_block_resized_;
    bool repair_block_resized_;
    bool payload_resized_;

    unsigned n_packets_;

    bool prev_block_timestamp_valid_;
    packet::stream_timestamp_t prev_block_timestamp_;
    packet::stream_timestamp_diff_t block_max_duration_;

    const size_t max_sbn_jump_;
    const packet::FecScheme fec_scheme_;

    status::StatusCode init_status_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_BLOCK_READER_H_
