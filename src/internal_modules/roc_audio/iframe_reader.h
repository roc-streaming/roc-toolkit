/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iframe_reader.h
//! @brief Frame reader interface.

#ifndef ROC_AUDIO_IFRAME_READER_H_
#define ROC_AUDIO_IFRAME_READER_H_

#include "roc_audio/frame.h"
#include "roc_core/attributes.h"
#include "roc_core/list_node.h"
#include "roc_packet/units.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Frame reading mode.
enum FrameReadMode {
    //! Read as much samples as possible.
    //!
    //! @remarks
    //!  If read encounters packet loss, returned frame will have gaps.
    //!  Gaps can be filled with zeros or something else, e.g. PLC can
    //!  fill gaps with interpolated data.
    //!
    //! @note
    //!  Returned size can be also capped by maximum buffer size or other
    //!  implementation-specific limitations.
    ModeHard,

    //! Stop reading when encountered a loss.
    //!
    //! @remarks
    //!  If read encounters packet loss, it stops and returns only samples
    //!  before the loss (if any).
    //!
    //! @note
    //!  Returned size can be also capped by maximum buffer size or other
    //!  implementation-specific limitations.
    ModeSoft
};

//! Frame reader interface.
class IFrameReader : public core::ListNode<> {
public:
    virtual ~IFrameReader();

    //! Read frame.
    //!
    //! Parameters:
    //!  - @p frame defines output frame, probably with pre-allocated buffer
    //!  - @p duration defines requested duration of output frame
    //!  - @p mode defines what to do in case of packet loss (see ReadMode)
    //!
    //! @note
    //!  - If frame does not have larger enough buffer, reader must allocate it
    //!    and attach to frame (using FrameFactory).
    //!  - If frame already has a buffer, reader may reslice it (i.e. shift slice
    //!    beginning or ending pointers within slice capacity), and write data to
    //!    it, but also is allowed to ignore it and replace with its own buffer.
    //!
    //! @returns
    //!  - If frame was successfully and completely read, returns status::StatusOK,
    //!    and sets @p frame duration to requested @p duration.
    //!  - If frame was partially read, returns status::StatusPart and sets @p frame
    //!    duration to a smaller value than requested @p duration.
    //!  - If @p ModeSoft is used, and there is no more samples before next loss,
    //!    returns status::StatusDrain.
    //!  - Otherwise, returns an error.
    //!
    //! @see status::StatusCode.
    virtual ROC_ATTR_NODISCARD status::StatusCode
    read(Frame& frame, packet::stream_timestamp_t duration, FrameReadMode mode) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IFRAME_READER_H_
