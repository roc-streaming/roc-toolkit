/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iplc.h
//! @brief PLC interface.

#ifndef ROC_AUDIO_IPLC_H_
#define ROC_AUDIO_IPLC_H_

#include "roc_audio/frame.h"
#include "roc_audio/sample_spec.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Packet loss concealment (PLC) interface.
//!
//! Used to mask the effects of packet loss when lost packets were
//! not recovered using FEC.
//!
//! Unlike FEC, which recovers original packet bit-to-bit (but may fail),
//! PLC is lossy as it uses interpolation. However usually it's still
//! better than silence, because distortion becomes less audible.
//!
//! IPlc is invoked by PlcReader. IPlc implements interpolation algorithm,
//! and PlcReader integrates it into receiver pipeline.
//!
//! Each frame, PlcReader invokes either process_history() (so that PLC can
//! remember previously played samples), or process_loss() (to ask PLC to
//! generate interpolated samples), depending on whether there's a loss.
//!
//! PLC implementation is allowed to use arbitrary PCM format, specified
//! by its sample_spec() method.
class IPlc : public core::ArenaAllocation {
public:
    //! Initialize.
    explicit IPlc(core::IArena& arena);

    //! Deinitialize.
    virtual ~IPlc();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const = 0;

    //! Sample specification expected by PLC.
    virtual SampleSpec sample_spec() const = 0;

    //! How many samples before lost frame are needed for interpolation.
    //! @remarks
    //!  - If it returns N, PLC reader will remember last N samples before the
    //!    gap. It will provide them to process_loss() via prev_frame argument.
    //!  - If it returns 0, prev_frame argument will be NULL.
    virtual packet::stream_timestamp_t lookbehind_len() = 0;

    //! How many samples after lost frame are needed for interpolation.
    //! @remarks
    //!  - If it returns N, PLC reader will try to read next N samples following
    //!    the gap. It will provide them to process_loss() via next_frame argument.
    //!  - If it returns 0, next_frame argument will be NULL.
    virtual packet::stream_timestamp_t lookahead_len() = 0;

    //! When next frame has no losses, PLC reader calls this method.
    //! PLC may remember samples to use it later for interpolation.
    virtual void process_history(Frame& hist_frame) = 0;

    //! When next frame is lost, PLC reader calls this method.
    //! PLC should fill the lost frame with the interpolated data.
    //! @remarks
    //!  - @p lost_frame is the frame to be filled with the interpolated data
    //!  - @p prev_frame is non-null only if lookbehind_len() returns non-zero;
    //!    in this case, prev_frame contains last N samples before the loss,
    //!    where N <= lookbehind_len()
    //!  - @p next_frame is non-null only if lookahead_len() returns non-zero,
    //!    and packets following the loss have already arrived;
    //!    in this case, next_frame contains next N samples after the loss,
    //!    where N <= lookahead_len()
    //!  - @p prev_frame may be shorter only in the very beginning of the stream,
    //!    when there are not enough samples before the loss
    //!  - @p next_frame may be shorter or even empty quite frequently,
    //!    depending on whether packets next to the loss already arrived
    virtual void
    process_loss(Frame& lost_frame, Frame* prev_frame, Frame* next_frame) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IPLC_H_
