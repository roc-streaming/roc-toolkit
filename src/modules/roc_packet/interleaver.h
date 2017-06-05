/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
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
#include "roc_core/byte_buffer.h"
#include "roc_core/noncopyable.h"

#include "roc_packet/ipacket.h"
#include "roc_packet/ipacket_writer.h"

namespace roc {
namespace packet {

//! Interleaves packets to transmit in pseudo random order.
class Interleaver : public packet::IPacketWriter, public core::NonCopyable<> {
public:
    //! Initialize.
    //! @remarks
    //!  Interleaver reorders packets passed to write() and writes
    //!  them to @p output. @p delay_len must not be greater than delay_max_.
    explicit Interleaver(packet::IPacketWriter&, const size_t delay_len);

    //! Write next packet.
    //! @remarks
    //!  Packets are written to internal buffer. Buffered packets are
    //!  then reordered and sent to output writer.
    virtual void write(const packet::IPacketPtr& packet);

    //! Send all buffered packets to output writer.
    void flush();

    //! Maximum delay between writing packet and moment we get it in output
    //! in terms of packets number.
    size_t window_size() const;

private:
    //! Initialize tx_seq_ to a new randomized sequence.
    void reinit_seq();

    packet::IPacketWriter& output_;
    // Number of packets in block.
    size_t delay_len_;
    //! Maximum possible number of packets in block.
    static const size_t delay_max_ = 32;
    size_t tx_seq_[delay_max_];
    // Delay line.
    core::Array<packet::IPacketPtr, delay_max_> pack_store_;

    size_t next_2_put_, next_2_send_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_INTERLEAVER_H_
