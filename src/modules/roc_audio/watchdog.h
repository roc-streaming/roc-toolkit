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

#include "roc_audio/ireader.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Watchdog.
//! @remarks
//!  Terminates session if during some period of time all frames has an empty flag.
class Watchdog : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input frame reader;
    //!  - @p timeout is maximum allowed period without new packets
    //!    before session termination;
    //!  - @p skip_window_sz is a window during which frames with a skip flag are
    //!       detected. If at least one frame has this flag, the session should be
    //!       terminated.
    Watchdog(IReader& reader, packet::timestamp_t timeout, size_t skip_window_sz);

    //! Read audio frame.
    //! @remarks
    //!  Updates stream state and reads frame from the input reader.
    virtual void read(Frame& frame);

    //! Update stream.
    //! @returns
    //!  false if all frames has an empty or skip flags during session timeout.
    bool update(packet::timestamp_t time);

private:
    IReader& reader_;

    const packet::timestamp_t timeout_;

    packet::timestamp_t update_time_;
    packet::timestamp_t read_time_;

    bool first_;
    bool alive_;

    packet::timestamp_t skip_time_;
    size_t frame_num_;
    size_t skip_window_sz_;
    bool total_skip_;
    bool curr_skip_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_WATCHDOG_H_
