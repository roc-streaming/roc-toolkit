/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/wrecker.h
//! @brief Packet wrecker.

#ifndef ROC_PACKET_WRECKER_H_
#define ROC_PACKET_WRECKER_H_

#include "roc_core/noncopyable.h"

#include "roc_packet/ipacket_writer.h"

namespace roc {
namespace packet {

//! Packet wrecker.
class Wrecker : public IPacketWriter, public core::NonCopyable<> {
public:
    //! Constructor.
    Wrecker(IPacketWriter& writer);

    //! Set packet loss rate.
    //! @remarks
    //!  @p rate is perecntage of packets to be lost in range [0; 100].
    void set_random_loss(size_t rate);

    //! Set packet delay rate.
    //! @remarks
    //!  @p rate is perecntage of packets to be delayed in range [0; 100].
    //!  @p ms is delay in milliseconds.
    void set_random_delay(size_t rate, size_t ms);

    //! Add packet.
    virtual void write(const IPacketPtr&);

private:
    IPacketWriter& writer_;
    size_t loss_rate_;
    size_t delay_rate_;
    size_t delay_ms_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_WRECKER_H_
