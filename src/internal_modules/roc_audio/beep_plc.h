/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/beep_plc.h
//! @brief Beep PLC.

#ifndef ROC_AUDIO_BEEP_PLC_H_
#define ROC_AUDIO_BEEP_PLC_H_

#include "roc_audio/frame_factory.h"
#include "roc_audio/iplc.h"
#include "roc_audio/plc_config.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! Beep "PLC".
//! Replaces lost samples with a loud beep.
//! Useful for debugging to distinguish losses easily.
class BeepPlc : public IPlc {
public:
    //! Initialize.
    BeepPlc(const PlcConfig& config,
            const SampleSpec& sample_spec,
            FrameFactory& frame_factory,
            core::IArena& arena);

    virtual ~BeepPlc();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Sample specification expected by PLC.
    virtual SampleSpec sample_spec() const;

    //! How many samples before lost frame are needed for interpolation.
    virtual packet::stream_timestamp_t lookbehind_len();

    //! How many samples after lost frame are needed for interpolation.
    virtual packet::stream_timestamp_t lookahead_len();

    //! When next frame has no losses, PLC reader calls this method.
    virtual void process_history(Frame& hist_frame);

    //! When next frame is lost, PLC reader calls this method.
    virtual void process_loss(Frame& lost_frame, Frame* prev_frame, Frame* next_frame);

private:
    const SampleSpec sample_spec_;
    uint32_t signal_pos_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_BEEP_PLC_H_
