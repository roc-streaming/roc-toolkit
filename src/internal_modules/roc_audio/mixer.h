/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/mixer.h
//! @brief Mixer.

#ifndef ROC_AUDIO_MIXER_H_
#define ROC_AUDIO_MIXER_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/iframe_reader.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/list.h"
#include "roc_core/noncopyable.h"
#include "roc_core/time.h"

namespace roc {
namespace audio {

//! Mixer.
//!
//! Mixes multiple input streams into one output stream.
//!
//! Features:
//!  - If requested duration is larger than maximum frame buffer size, mixer
//!    splits request into multiple read operations and concatenates results.
//!
//!  - If pipeline element reports a partial read (StatusPart), mixer repeats
//!    reads until requested amount of samples is accumulated.
//!
//!  - If pipeline element reports temporary lack of data (StatusDrain), mixer
//!    skips this element during current read.
//!
//!    (In other words, StatusPart and StatusDrain never leave mixer. Mixer
//!    always returns as much samples as requested).
//!
//!  - If timestamps are enabled, mixer computes capture timestamp of output
//!    frame as the average capture timestamps of all mixed input frames.
//!
//!    (This makes sense only when all inputs are synchronized and their
//!    timestamps are close to each other).
class Mixer : public IFrameReader, public core::NonCopyable<> {
public:
    //! Initialize.
    //! @p enable_timestamps defines whether to enable calculation of capture timestamps.
    Mixer(FrameFactory& frame_factory,
          const SampleSpec& sample_spec,
          bool enable_timestamps);

    //! Check if the object was successfully constructed.
    status::StatusCode init_status() const;

    //! Add input reader.
    void add_input(IFrameReader&);

    //! Remove input reader.
    void remove_input(IFrameReader&);

    //! Read audio frame.
    //! @remarks
    //!  Reads samples from every input reader, mixes them, and fills @p frame
    //!  with the result.
    //! @note
    //!  Requested @p duration is allowed to be larger than maximum buffer
    //!  size, but only if @p frame has pre-allocated buffer big enough.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration);

private:
    status::StatusCode mix_all_(sample_t* out_data,
                                size_t out_size,
                                unsigned& out_flags,
                                core::nanoseconds_t& out_cts);

    status::StatusCode mix_one_(IFrameReader& frame_reader,
                                sample_t* out_data,
                                size_t out_size,
                                unsigned& out_flags,
                                core::nanoseconds_t& out_cts);

    FrameFactory& frame_factory_;
    core::List<IFrameReader, core::NoOwnership> frame_readers_;

    FramePtr in_frame_;

    const SampleSpec sample_spec_;
    const size_t max_read_;
    const bool enable_timestamps_;

    status::StatusCode init_status_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_MIXER_H_
