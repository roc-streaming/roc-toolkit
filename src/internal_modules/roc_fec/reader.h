/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/reader.h
//! @brief FEC reader.

#ifndef ROC_FEC_READER_H_
#define ROC_FEC_READER_H_

#include "roc_core/array.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_fec/iblock_decoder.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/sorted_queue.h"

namespace roc {
namespace fec {

//! FEC reader parameters.
struct ReaderConfig {
    //! Maximum allowed source block number jump.
    size_t max_sbn_jump;

    ReaderConfig()
        : max_sbn_jump(100) {
    }
};

//! FEC reader.
class Reader : public packet::IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p config contains FEC scheme parameters
    //!  - @p decoder specifies FEC codec implementation;
    //!  - @p source_reader specifies input queue with data packets;
    //!  - @p repair_reader specifies input queue with FEC packets;
    //!  - @p parser specifies packet parser for restored packets.
    //!  - @p allocator is used to initialize a packet array
    Reader(const ReaderConfig& config,
           packet::FecScheme fec_scheme,
           IBlockDecoder& decoder,
           packet::IReader& source_reader,
           packet::IReader& repair_reader,
           packet::IParser& parser,
           packet::PacketPool& packet_pool,
           core::IAllocator& allocator);

    //! Check if object is successfully constructed.
    bool valid() const;

    //! Did decoder catch block beginning?
    bool started() const;

    //! Is decoder alive?
    bool alive() const;

    //! Read packet.
    //! @remarks
    //!  When a packet loss is detected, try to restore it from repair packets.
    virtual packet::PacketPtr read();

private:
    packet::PacketPtr read_();

    packet::PacketPtr get_first_packet_();
    packet::PacketPtr get_next_packet_();

    void next_block_();
    void try_repair_();

    packet::PacketPtr parse_repaired_packet_(const core::Slice<uint8_t>& buffer);

    void fetch_packets_();

    void fill_block_();
    void fill_source_block_();
    void fill_repair_block_();

    bool process_source_packet_(const packet::PacketPtr&);
    bool process_repair_packet_(const packet::PacketPtr&);

    bool validate_fec_packet_(const packet::PacketPtr&);
    bool validate_sbn_sequence_(const packet::PacketPtr&);

    bool validate_incoming_source_packet_(const packet::PacketPtr&);
    bool validate_incoming_repair_packet_(const packet::PacketPtr&);

    bool can_update_payload_size_(size_t);
    bool can_update_source_block_size_(size_t);
    bool can_update_repair_block_size_(size_t);

    bool update_payload_size_(size_t);
    bool update_source_block_size_(size_t);
    bool update_repair_block_size_(size_t);

    void drop_repair_packets_from_prev_blocks_();

    IBlockDecoder& decoder_;

    packet::IReader& source_reader_;
    packet::IReader& repair_reader_;
    packet::IParser& parser_;
    packet::PacketPool& packet_pool_;

    packet::SortedQueue source_queue_;
    packet::SortedQueue repair_queue_;

    core::Array<packet::PacketPtr> source_block_;
    core::Array<packet::PacketPtr> repair_block_;

    bool valid_;

    bool alive_;
    bool started_;
    bool can_repair_;

    size_t next_packet_;
    packet::blknum_t cur_sbn_;

    size_t payload_size_;

    bool source_block_resized_;
    bool repair_block_resized_;
    bool payload_resized_;

    unsigned n_packets_;

    const size_t max_sbn_jump_;
    const packet::FecScheme fec_scheme_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_READER_H_
