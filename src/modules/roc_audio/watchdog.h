/*
 * Copyright (c) 2018 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/watchdog.h
//! @brief Watchdog.

#ifndef ROC_AUDIO_WATCHDOG_H_
#define ROC_AUDIO_WATCHDOG_H_

#include "roc_core/noncopyable.h"
#include "roc_audio/ireader.h"
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
    //!  - @p timeout is maximum allowed period with empty frames
    //!    before session termination;
    Watchdog(IReader& reader, packet::timestamp_t timeout);

    //! Read audio frame.
    //! @remarks
    //!  Updates stream state and reads frame from the input reader.
    virtual void read(Frame& frame);

    //! Update stream.
    //! @returns
    //!  false if all frames has an empty flag during the session timeout.
    bool update(packet::timestamp_t time);

private:
    void check_frame_empty_(const Frame& frame);
    bool has_all_frames_empty_(const packet::timestamp_t update_time) const;

    IReader& reader_;

    const packet::timestamp_t timeout_;

    packet::timestamp_t update_time_;
    packet::timestamp_t read_time_;

    bool first_;
    bool alive_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_WATCHDOG_H_
