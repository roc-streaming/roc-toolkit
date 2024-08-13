/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iframe_encoder.h
//! @brief Audio frame encoder interface.

#ifndef ROC_AUDIO_IFRAME_ENCODER_H_
#define ROC_AUDIO_IFRAME_ENCODER_H_

#include "roc_audio/sample.h"
#include "roc_core/stddefs.h"
#include "roc_packet/packet.h"
#include "roc_packet/units.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Audio frame encoder interface.
class IFrameEncoder : public core::ArenaAllocation {
public:
    //! Initialize.
    explicit IFrameEncoder(core::IArena& arena);

    //! Deinitialize.
    virtual ~IFrameEncoder();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const = 0;

    //! Get encoded frame size in bytes for given number of samples (per channel).
    virtual size_t encoded_byte_count(size_t n_samples) const = 0;

    //! Start encoding a new frame.
    //!
    //! @remarks
    //!  After this call, write_samples() will store samples to the given @p frame_data
    //!  until @p frame_size bytes are written or end_frame() is called.
    virtual void begin_frame(void* frame_data, size_t frame_size) = 0;

    //! Write samples into current frame.
    //!
    //! @b Parameters
    //!  - @p samples - samples to be encoded
    //!  - @p n_samples - number of samples to be encoded (per channel)
    //!
    //! @remarks
    //!  Encodes samples and writes to the current frame.
    //!
    //! @returns
    //!  number of samples encoded per channel. The returned value can be fewer than
    //!  @p n_samples if the frame is full and no more samples can be written to it.
    //!
    //! @pre
    //!  This method may be called only between begin_frame() and end_frame().
    virtual size_t write_samples(const sample_t* samples, size_t n_samples) = 0;

    //! Finish encoding current frame.
    //!
    //! @remarks
    //!  After this call, the frame is fully encoded and no more samples is written
    //!  to the frame. A new frame should be started by calling begin_frame().
    virtual void end_frame() = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IFRAME_ENCODER_H_
