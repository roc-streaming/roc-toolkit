/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iframe_decoder.h
//! @brief Audio frame decoder interface.

#ifndef ROC_AUDIO_IFRAME_DECODER_H_
#define ROC_AUDIO_IFRAME_DECODER_H_

#include "roc_audio/sample.h"
#include "roc_core/stddefs.h"
#include "roc_packet/packet.h"
#include "roc_packet/units.h"
#include "roc_status/status_code.h"

namespace roc {
namespace audio {

//! Audio frame decoder interface.
class IFrameDecoder : public core::ArenaAllocation {
public:
    //! Initialize.
    explicit IFrameDecoder(core::IArena& arena);

    //! Deinitialize.
    virtual ~IFrameDecoder();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const = 0;

    //! Get decoded stream position.
    //!
    //! @returns
    //!  the position of the next sample that will be retrieved by read_samples().
    //!
    //! @remarks
    //!  The decoded stream position is affected by begin_frame(), read_samples(), and
    //!  drop_samples() methods. begin_frame() changes it according to the provided frame
    //!  position, however it depends on the implementation how exactly. read_samples()
    //!  and drop_samples() increase it by the number of samples they returned.
    virtual packet::stream_timestamp_t position() const = 0;

    //! Get number of samples available for decoding.
    //!
    //! @returns
    //!  number of available samples per channel, or zero if there are no more
    //!  samples in the current frame, or if begin_frame() was not called yet.
    //!
    //! @remarks
    //!  The number of samples available is affected by begin_frame(), read_samples(), and
    //!  drop_samples(), and end_frame() methods. begin_frame() resets it according to the
    //!  provided frame size, however it depends on the implementation how exactly.
    //!  end_frame() resets it to zero. read_samples() and drop_samples() decrease it by
    //!  the number of samples they returned.
    virtual packet::stream_timestamp_t available() const = 0;

    //! Get number of samples per channel that can be decoded from given frame.
    virtual size_t decoded_sample_count(const void* frame_data,
                                        size_t frame_size) const = 0;

    //! Start decoding a new frame.
    //!
    //! @remarks
    //!  After this call, read_samples() will retrieve samples from given @p frame_data,
    //!  until
    //!  @p frame_size bytes are read or end_frame() is called.
    //!
    //! @note
    //!  @p frame_position defines the position of the frame in the encoded stream.
    //!  Decoder updates the decoded stream position according to @p frame_position,
    //!  but not necessary to the same value. Encoded and decoded stream positions
    //!  may be slightly different, depending on the codec implementation.
    virtual void begin_frame(packet::stream_timestamp_t frame_position,
                             const void* frame_data,
                             size_t frame_size) = 0;

    //! Read samples from current frame.
    //!
    //! @b Parameters
    //!  - @p samples - buffer to write decoded samples to
    //!  - @p n_samples - number of samples to be decoded (per channel)
    //!
    //! @remarks
    //!  Decodes samples from the current frame and writes them to the provided buffer.
    //!
    //! @returns
    //!  number of samples decoded per channel. The returned value can be fewer than
    //!  @p n_samples if there are no more samples in the current frame.
    //!
    //! @pre
    //!  This method may be called only between begin_frame() and end_frame().
    virtual size_t read_samples(sample_t* samples, size_t n_samples) = 0;

    //! Shift samples from current frame.
    //!
    //! @b Parameters
    //!  - @p n_samples - number of samples to shift per channel
    //!
    //! @remarks
    //!  Shifts the given number of samples from the left, as if read_samples() was called
    //!  and the result was dropped.
    //!
    //! @returns
    //!  number of samples shifted per channel. The returned value can be fewer than
    //!  @p n_samples if there are no more samples in the current frame.
    //!
    //! @pre
    //!  This method may be called only between begin_frame() and end_frame().
    virtual size_t drop_samples(size_t n_samples) = 0;

    //! Finish decoding current frame.
    //!
    //! @remarks
    //!  After this call, the frame can't be read or shifted anymore. A new frame
    //!  should be started by calling begin_frame().
    virtual void end_frame() = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IFRAME_DECODER_H_
