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
#include "roc_status/code_to_str.h"

namespace roc {
namespace audio {

//! Watchdog parameters.
struct WatchdogConfig {
    //! Timeout for the lack of packets, nanoseconds.
    //! @remarks
    //!  Maximum allowed period during which every frame is blank. After this period,
    //!  the session is terminated. This mechanism allows to detect dead, hanging, or
    //!  broken clients.
    //! @note
    //!  If zero, default value is used.
    //!  If negative, the check is disabled.
    core::nanoseconds_t no_playback_timeout;

    //! Timeout for frequent stuttering, nanoseconds.
    //! @remarks
    //!  Maximum allowed period during which every drop detection window overlaps with
    //!  at least one frame which caused packet drops and with at least one frame which
    //!  is incomplete (it may be the same frame). After this period, the session is
    //!  terminated. This mechanism allows to detect the vicious circle when all client
    //!  packets are a bit late and we are constantly dropping them producing unpleasant
    //!  noise.
    //! @note
    //!  If zero, default value is used.
    //!  If negative, the check is disabled.
    core::nanoseconds_t choppy_playback_timeout;

    //! Window size of detecting stuttering, nanoseconds.
    //! @see choppy_playback_timeout
    //! @note
    //!  If zero, default value is used.
    core::nanoseconds_t choppy_playback_window;

    //! Duration of the warmup phase in the beginning, nanoseconds
    //! @remarks
    //!  During the warmup phase blank_timeout is not triggered. After this period last
    //!  position before blank frames is set to the current position. Warmup can also
    //!  be terminated in case a non-blank frame occurs during it. This mechanism allows
    //!  watchdog to work with latency longer than no_playback_timeout. Usually is equal
    //!  to target_latency.
    //! @note
    //!  If zero, default value is used.
    //!  If negative, warmup phase is disabled.
    core::nanoseconds_t warmup_duration;

    //! Frame status window size for logging, number of frames.
    //! @remarks
    //!  Used for debug logging. Set to zero to disable.
    //! @note
    //!  If zero, default value is used.
    size_t frame_status_window;

    //! Initialize config with default values.
    WatchdogConfig()
        : no_playback_timeout(0)
        , choppy_playback_timeout(0)
        , choppy_playback_window(0)
        , warmup_duration(0)
        , frame_status_window(20) {
    }

    //! Automatically fill missing settings.
    ROC_ATTR_NODISCARD bool deduce_defaults(const core::nanoseconds_t default_latency,
                                            const core::nanoseconds_t target_latency);
};

//! Watchdog.
//! @remarks
//!  Terminates session if it is considered dead or corrupted.
class Watchdog : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialize.
    Watchdog(IFrameReader& reader,
             const SampleSpec& sample_spec,
             const WatchdogConfig& config,
             core::IArena& arena);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Read audio frame.
    //! @remarks
    //!  Updates stream state and reads frame from the input reader.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode);

private:
    bool update_(Frame& frame);

    void update_blank_timeout_(const Frame& frame,
                               packet::stream_timestamp_t next_read_pos);
    bool check_blank_timeout_() const;

    void update_drops_timeout_(const Frame& frame,
                               packet::stream_timestamp_t next_read_pos);
    bool check_drops_timeout_();

    void update_warmup_();

    void update_status_(const Frame& frame);
    void flush_status_();

    IFrameReader& reader_;

    const SampleSpec sample_spec_;

    packet::stream_timestamp_t max_blank_duration_;
    packet::stream_timestamp_t max_drops_duration_;
    packet::stream_timestamp_t drops_detection_window_;

    packet::stream_timestamp_t curr_read_pos_;
    packet::stream_timestamp_t last_pos_before_blank_;
    packet::stream_timestamp_t last_pos_before_drops_;

    packet::stream_timestamp_t warmup_duration_;
    bool in_warmup_;

    unsigned curr_window_flags_;

    core::Array<char> status_;
    size_t status_pos_;
    bool show_status_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_WATCHDOG_H_
