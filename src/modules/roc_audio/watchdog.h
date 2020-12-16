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
#include "roc_core/array.h"
#include "roc_core/iallocator.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Watchdog parameters.
struct WatchdogConfig {
    //! Timeout for the lack of packets, nanoseconds.
    //! @remarks
    //!  Maximum allowed period during which every frame is blank. After this period,
    //!  the session is terminated. This mechanism allows to detect dead, hanging, or
    //!  broken clients. Set to zero to disable.
    core::nanoseconds_t no_playback_timeout;

    //! Timeout for frequent breakages, nanoseconds.
    //! @remarks
    //!  Maximum allowed period during which every drop detection window overlaps with
    //!  at least one frame which caused packet drops and with at least one frame which
    //!  is incomplete (it may be the same frame). After this period, the session is
    //!  terminated. This mechanism allows to detect the vicious circle when all client
    //!  packets are a bit late and we are constantly dropping them producing unpleasant
    //!  noise. Set to zero to disable.
    core::nanoseconds_t broken_playback_timeout;

    //! Breakage detection window, nanoseconds.
    //! @see broken_playback_timeout.
    core::nanoseconds_t breakage_detection_window;

    //! Frame status window size for logging, number of frames.
    //! @remarks
    //!  Used for debug logging. Set to zero to disable.
    size_t frame_status_window;

    //! Initialize config with default values.
    WatchdogConfig()
        : no_playback_timeout(2 * core::Second)
        , broken_playback_timeout(2 * core::Second)
        , breakage_detection_window(300 * core::Millisecond)
        , frame_status_window(20) {
    }
};

//! Watchdog.
//! @remarks
//!  Terminates session if it is considered dead or corrupted.
class Watchdog : public IReader, public core::NonCopyable<> {
public:
    //! Initialize.
    Watchdog(IReader& reader,
             size_t num_channels,
             const WatchdogConfig& config,
             size_t sample_rate,
             core::IAllocator& allocator);

    //! Check if object is successfully constructed.
    bool valid() const;

    //! Read audio frame.
    //! @remarks
    //!  Updates stream state and reads frame from the input reader.
    virtual ssize_t read(Frame& frame);

    //! Update stream.
    //! @returns
    //!  false if during the session timeout each frame has an empty flag or the maximum
    //!  allowed number of consecutive windows that can contain frames that aren't fully
    //!  filled and contain dropped packets was exceeded.
    bool update();

private:
    void update_blank_timeout_(const Frame& frame, packet::timestamp_t next_read_pos);
    bool check_blank_timeout_() const;

    void update_drops_timeout_(const Frame& frame, packet::timestamp_t next_read_pos);
    bool check_drops_timeout_();

    void update_status_(const Frame& frame);
    void flush_status_();

    IReader& reader_;

    const size_t num_channels_;

    const packet::timestamp_t max_blank_duration_;
    const packet::timestamp_t max_drops_duration_;
    const packet::timestamp_t drop_detection_window_;

    packet::timestamp_t curr_read_pos_;
    packet::timestamp_t last_pos_before_blank_;
    packet::timestamp_t last_pos_before_drops_;

    unsigned curr_window_flags_;

    core::Array<char> status_;
    size_t status_pos_;
    bool status_show_;

    bool alive_;
    bool valid_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_WATCHDOG_H_
