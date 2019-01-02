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

#include "roc_audio/ireader.h"
#include "roc_core/noncopyable.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Watchdog.
//! @remarks
//!  Terminates session if it is considered dead or corrupted.
class Watchdog : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input frame reader.
    //!  - @p channels defines a set of channels in the output frames.
    //!  - @p max_silence_duration is the maximum allowed period during which all frames
    //!    are empty.
    //!  - @p max_drops_duration is the maximum allowed period during which every drop
    //!    detection window overlaps with a frame which contains drops.
    //!  - @p drop_detection_window is the size of the drop detection window.
    Watchdog(IReader& reader,
             const size_t num_channels,
             packet::timestamp_t max_silence_duration,
             packet::timestamp_t max_drops_duration,
             packet::timestamp_t drop_detection_window);

    //! Read audio frame.
    //! @remarks
    //!  Updates stream state and reads frame from the input reader.
    virtual void read(Frame& frame);

    //! Update stream.
    //! @returns
    //!  false if during the session timeout each frame has an empty flag or the maximum
    //!  allowed number of consecutive windows that can contain frames that aren't fully
    //!  filled and contain dropped packets was exceeded.
    bool update(packet::timestamp_t time);

private:
    void init_silence_timeout_(packet::timestamp_t update_time);
    void update_silence_timeout_(const Frame& frame);
    bool check_silence_timeout_() const;

    void update_drops_timeout_(const Frame& frame, packet::timestamp_t next_read_pos);
    bool check_drops_timeout_();

    IReader& reader_;

    const size_t num_channels_;

    const packet::timestamp_t max_silence_duration_;
    const packet::timestamp_t max_drops_duration_;
    const packet::timestamp_t drop_detection_window_;

    bool first_update_;
    bool alive_;

    packet::timestamp_t curr_read_pos_;
    packet::timestamp_t last_update_time_;
    packet::timestamp_t last_update_before_silence_;
    packet::timestamp_t last_read_before_drops_;

    bool drop_in_curr_window_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_WATCHDOG_H_
