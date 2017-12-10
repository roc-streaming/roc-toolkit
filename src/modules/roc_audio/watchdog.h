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
//!  Terminates session if during some period of time all frames were empty.
class Watchdog : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //!
    //! @b Parameters
    //!  - @p reader is input frame reader.
    //!  - @p channels defines a set of channels in the output frames.
    //!  - @p timeout is maximum allowed period with empty frames
    //!    before session termination.
    //!  - @p drop_window_sz is a window size during which not fully filled frames that
    //!    have dropped packets are detected.
    //!  - @p max_drop_window_num is the maximum allowed number of consecutive windows
    //!    that can contain frames that aren't fully filled and contain dropped packets.
    Watchdog(IReader& reader,
             const size_t num_channels,
             packet::timestamp_t timeout,
             packet::timestamp_t drop_window_sz,
             packet::timestamp_t max_drop_window_num);

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
    bool has_all_frames_empty_(const packet::timestamp_t update_time) const;
    bool has_dropped_frames_() const;
    void check_frame_empty_(const Frame& frame);
    void check_frame_has_dropped_packets_(const Frame& frame);
    void add_drop_samples_(const size_t num_samples);
    bool check_drop_window_exceeded_();
    void update_drop_window_(const size_t num_samples);
    void reset_drop_window_();

    IReader& reader_;

    const packet::timestamp_t timeout_;

    packet::timestamp_t update_time_;
    packet::timestamp_t read_time_;

    bool first_;
    bool alive_;

    const size_t num_channels_;

    packet::timestamp_t max_drop_window_num_;
    packet::timestamp_t drop_window_sz_;
    packet::timestamp_t drop_samples_;
    packet::timestamp_t frame_samples_;

    bool total_drop_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_WATCHDOG_H_
