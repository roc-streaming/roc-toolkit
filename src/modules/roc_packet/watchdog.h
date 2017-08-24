/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/watchdog.h
//! @brief Watchdog.

#ifndef ROC_PACKET_WATCHDOG_H_
#define ROC_PACKET_WATCHDOG_H_

#include "roc_core/noncopyable.h"
#include "roc_packet/ireader.h"

namespace roc {
namespace packet {

//! Watchdog.
//! @remarks
//!  Terminates session if there are no new packets during a long period of time.
class Watchdog : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input packet reader; packets from @p reader
    //!    are returned from read();
    //!  - @p timeout is maximum allowed period without new packets
    //!    before session termination;
    Watchdog(IReader& reader, timestamp_t timeout);

    //! Read next packet.
    //! @remarks
    //!  Updates stream state and returns next packet from the input reader.
    virtual PacketPtr read();

    //! Update stream.
    //! @returns
    //!  false if there are no packets during session timeout.
    bool update(timestamp_t time);

private:
    IReader& reader_;

    const timestamp_t timeout_;

    timestamp_t update_time_;
    timestamp_t read_time_;

    bool first_;
    bool alive_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_WATCHDOG_H_
