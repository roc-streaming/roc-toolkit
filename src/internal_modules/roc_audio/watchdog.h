/*
 * Copyright (c) 2018 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/watchdog.h
//! @brief Watchdog.

#ifndef ROC_AUDIO_WATCHDOG_H_
#define ROC_AUDIO_WATCHDOG_H_

#include "roc_audio/iframe_reader.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/array.h"
#include "roc_core/attributes.h"
#include "roc_core/iarena.h"
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

    //! Timeout for frequent stuttering, nanoseconds.
    //! @remarks
    //!  Maximum allowed period during which every drop detection window overlaps with
    //!  at least one frame which caused packet drops and with at least one frame which
    //!  is incomplete (it may be the same frame). After this period, the session is
    //!  terminated. This mechanism allows to detect the vicious circle when all client
    //!  packets are a bit late and we are constantly dropping them producing unpleasant
    //!  noise. Set to zero to disable.
    core::nanoseconds_t choppy_playback_timeout;

    //! Window size of detecting stuttering, nanoseconds.
    //! @see
    //!  choppy_playback_timeout
    core::nanoseconds_t choppy_playback_window;

    //! Frame status window size for logging, number of frames.
    //! @remarks
    //!  Used for debug logging. Set to zero to disable.
    size_t frame_status_window;

    //! Initialize config with default values.
    WatchdogConfig()
        : no_playback_timeout(2 * core::Second)
        , choppy_playback_timeout(2 * core::Second)
        , choppy_playback_window(300 * core::Millisecond)
        , frame_status_window(20) {
    }

    //! Automatically deduce choppy_playback_window from choppy_playback_timeout.
    void deduce_choppy_playback_window(core::nanoseconds_t timeout) {
        choppy_playback_window = std::min(300 * core::Millisecond, timeout / 4);
    }
};

//! Watchdog.
//! @remarks
//!  Terminates session if it is considered dead or corrupted.
class Watchdog : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialize.
    Watchdog(IFrameReader& reader,
             const audio::SampleSpec& sample_spec,
             const WatchdogConfig& config,
             core::IArena& arena);

    //! Check if object is successfully constructed.
    bool is_valid() const;

    //! Check if stream is still alive.
    //! @returns
    //!  false if during the session timeout each frame has an empty flag or the maximum
    //!  allowed number of consecutive windows that can contain frames that aren't fully
    //!  filled and contain dropped packets was exceeded.
    bool is_alive() const;

    //! Read audio frame.
    //! @remarks
    //!  Updates stream state and reads frame from the input reader.
    virtual bool read(Frame& frame);

private:
    void update_blank_timeout_(const Frame& frame,
                               packet::stream_timestamp_t next_read_pos);
    bool check_blank_timeout_() const;

    void update_drops_timeout_(const Frame& frame,
                               packet::stream_timestamp_t next_read_pos);
    bool check_drops_timeout_();

    void update_status_(const Frame& frame);
    void flush_status_();

    IFrameReader& reader_;

    const audio::SampleSpec sample_spec_;

    const packet::stream_timestamp_t max_blank_duration_;
    const packet::stream_timestamp_t max_drops_duration_;
    const packet::stream_timestamp_t drop_detection_window_;

    packet::stream_timestamp_t curr_read_pos_;
    packet::stream_timestamp_t last_pos_before_blank_;
    packet::stream_timestamp_t last_pos_before_drops_;

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
