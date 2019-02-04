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
#include "roc_fec/config.h"
#include "roc_fec/idecoder.h"
#include "roc_packet/iparser.h"
#include "roc_packet/ireader.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_pool.h"
#include "roc_packet/sorted_queue.h"

namespace roc {
namespace fec {

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
    Reader(const Config& config,
           IDecoder& decoder,
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
    packet::PacketPtr get_next_packet_();

    void next_block_();
    void try_repair_();
    bool check_packet_(const packet::PacketPtr&);

    void fetch_packets_();
    void update_packets_();

    void update_source_packets_();
    void update_repair_packets_();

    void drop_repair_packets_from_prev_blocks_();

    IDecoder& decoder_;

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

    bool has_source_;
    packet::source_t source_;

    unsigned n_packets_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_READER_H_
