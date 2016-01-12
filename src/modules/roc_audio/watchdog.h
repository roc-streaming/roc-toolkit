/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/watchdog.h
//! @brief Watchdog.

#ifndef ROC_AUDIO_WATCHDOG_H_
#define ROC_AUDIO_WATCHDOG_H_

#include "roc_config/config.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/ipacket_reader.h"
#include "roc_packet/iaudio_packet.h"
#include "roc_audio/ituner.h"

namespace roc {
namespace audio {

//! Watchdog.
//!
//! Triggers undesirable stream state and terminates rendering:
//!  - if there are no new packets during long period;
//!  - if long timestamp or seqnum jump occured.
class Watchdog : public ITuner, public packet::IPacketReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input packet reader; packets from @p reader
    //!    are returned from read();
    //!  - @p timeout is maximum allowed number of renderer ticks
    //!    without new packets before renderer termination.
    Watchdog(packet::IPacketReader& reader,
             size_t timeout = ROC_CONFIG_DEFAULT_SESSION_TIMEOUT);

    //! Update stream.
    //! @returns
    //!  false if stream is broken and rendering should terminate.
    virtual bool update();

    //! Read next packet.
    //! @remarks
    //!  updates stream state and returns next packet from input reader.
    virtual packet::IPacketConstPtr read();

private:
    bool detect_jump_(const packet::IPacketConstPtr&);

    packet::IPacketReader& reader_;
    packet::IPacketConstPtr prev_;

    size_t timeout_;
    size_t countdown_;

    bool has_packets_;
    bool alive_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_WATCHDOG_H_
