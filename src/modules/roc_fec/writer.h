/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_fec/writer.h
//! @brief FEC writer.

#ifndef ROC_FEC_WRITER_H_
#define ROC_FEC_WRITER_H_

#include "roc_core/array.h"
#include "roc_core/buffer_pool.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/slice.h"
#include "roc_fec/iblock_encoder.h"
#include "roc_packet/icomposer.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"
#include "roc_packet/packet_pool.h"

namespace roc {
namespace fec {

//! FEC writer parameters.
struct WriterConfig {
    //! Number of data packets in block.
    size_t n_source_packets;

    //! Number of FEC packets in block.
    size_t n_repair_packets;

    WriterConfig()
        : n_source_packets(20)
        , n_repair_packets(10) {
    }
};

//! FEC writer.
class Writer : public packet::IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p config contains FEC scheme parameters
    //!  - @p encoder is used to encode repair packets
    //!  - @p writer is used to write source and repair packets
    //!  - @p source_composer is used to format source packets
    //!  - @p repair_composer is used to format repair packets
    //!  - @p packet_pool is used to allocate repair packets
    //!  - @p buffer_pool is used to allocate buffers for repair packets
    //!  - @p allocator is used to initialize a packet array
    Writer(const WriterConfig& config,
           packet::FecScheme fec_scheme,
           IBlockEncoder& encoder,
           packet::IWriter& writer,
           packet::IComposer& source_composer,
           packet::IComposer& repair_composer,
           packet::PacketPool& packet_pool,
           core::BufferPool<uint8_t>& buffer_pool,
           core::IAllocator& allocator);

    //! Check if object is successfully constructed.
    bool valid() const;

    //! Check if writer is still working.
    bool alive() const;

    //! Set number of source packets per block.
    bool resize(size_t sblen, size_t rblen);

    //! Write packet.
    //! @remarks
    //!  - writes the given source packet to the output writer
    //!  - generates repair packets and also writes them to the output writer
    virtual void write(const packet::PacketPtr&);

private:
    bool begin_block_(const packet::PacketPtr& pp);
    void end_block_();
    void next_block_();

    bool apply_sizes_(size_t sblen, size_t rblen, size_t payload_size);

    void write_source_packet_(const packet::PacketPtr&);
    void make_repair_packets_();
    packet::PacketPtr make_repair_packet_(packet::seqnum_t n);
    void encode_repair_packets_();
    void compose_repair_packets_();
    void write_repair_packets_();
    void fill_packet_fec_fields_(const packet::PacketPtr& packet, packet::seqnum_t n);

    void validate_fec_packet_(const packet::PacketPtr&);
    bool validate_source_packet_(const packet::PacketPtr&);

    size_t cur_sblen_;
    size_t next_sblen_;

    size_t cur_rblen_;
    size_t next_rblen_;

    size_t cur_payload_size_;

    IBlockEncoder& encoder_;
    packet::IWriter& writer_;

    packet::IComposer& source_composer_;
    packet::IComposer& repair_composer_;

    packet::PacketPool& packet_pool_;
    core::BufferPool<uint8_t>& buffer_pool_;

    core::Array<packet::PacketPtr> repair_block_;

    bool first_packet_;

    packet::blknum_t cur_sbn_;
    packet::seqnum_t cur_block_repair_sn_;

    size_t cur_packet_;

    const packet::FecScheme fec_scheme_;

    bool valid_;
    bool alive_;
};

} // namespace fec
} // namespace roc

#endif // ROC_FEC_WRITER_H_
