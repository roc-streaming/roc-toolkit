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

#include "roc_config/config.h"
#include "roc_core/noncopyable.h"

#include "roc_packet/ipacket_reader.h"
#include "roc_packet/ipacket.h"
#include "roc_packet/imonitor.h"

namespace roc {
namespace packet {

//! Watchdog.
//!
//! Triggers undesirable stream state and terminates rendering:
//!  - if there are no new packets during long period;
//!  - if long timestamp or seqnum jump occured.
class Watchdog : public IMonitor, public IPacketReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input packet reader; packets from @p reader
    //!    are returned from read();
    //!  - @p timeout is maximum allowed number of renderer ticks
    //!    without new packets before renderer termination;
    //!  - @p rate is allowed rate for input packets; packets with
    //!    other rate are dropped.
    Watchdog(IPacketReader& reader,
             size_t timeout = ROC_CONFIG_DEFAULT_SESSION_TIMEOUT,
             size_t rate = ROC_CONFIG_DEFAULT_SAMPLE_RATE);

    //! Update stream.
    //! @returns
    //!  false if stream is broken and rendering should terminate.
    virtual bool update();

    //! Read next packet.
    //! @remarks
    //!  updates stream state and returns next packet from input reader.
    virtual IPacketConstPtr read();

private:
    bool detect_jump_(const IPacketConstPtr&);

    IPacketReader& reader_;
    IPacketConstPtr prev_;

    const size_t rate_;
    size_t timeout_;
    size_t countdown_;

    bool has_packets_;
    bool alive_;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_WATCHDOG_H_
