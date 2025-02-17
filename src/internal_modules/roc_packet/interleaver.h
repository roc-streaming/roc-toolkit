/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/interleaver.h
//! @brief Interleaves packets before transmit.

#ifndef ROC_PACKET_INTERLEAVER_H_
#define ROC_PACKET_INTERLEAVER_H_

#include "roc_core/array.h"
#include "roc_core/iarena.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/iwriter.h"
#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Interleaves packets to transmit them in pseudo random order.
class Interleaver : public IWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    //! @remarks
    //!  Interleaver reorders packets passed to write() and writes
    //!  them to @p output.
    Interleaver(IWriter& writer, core::IArena& arena, size_t block_size);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Write next packet.
    //! @remarks
    //!  Packets are written to internal buffer. Buffered packets are
    //!  then reordered and sent to output writer.
    virtual ROC_ATTR_NODISCARD status::StatusCode write(const PacketPtr& packet);

    //! Send all buffered packets to output writer.
    ROC_ATTR_NODISCARD status::StatusCode flush();

    //! Maximum delay between writing packet and moment we get it in output
    //! in terms of packets number.
    size_t block_size() const;

private:
    //! Initialize tx_seq_ to a new randomized sequence.
    void reinit_seq_();

    // Output writer.
    IWriter& writer_;

    // Number of packets in block.
    size_t block_size_;

    // Output sequence.
    core::Array<size_t> send_seq_;

    // Delay line.
    core::Array<PacketPtr> packets_;

    size_t next_2_put_;
    size_t next_2_send_;

    status::StatusCode init_status_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_INTERLEAVER_H_
