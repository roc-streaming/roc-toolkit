/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/idecoder.h
//! @brief Audio decoder interface.

#ifndef ROC_AUDIO_IDECODER_H_
#define ROC_AUDIO_IDECODER_H_

#include "roc_audio/units.h"
#include "roc_core/stddefs.h"
#include "roc_packet/packet.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Audio decoder interface.
class IDecoder {
public:
    virtual ~IDecoder();

    //! Start decoding a new packet.
    //!
    //! After this call, decoder will report position and sequentially read samples
    //! from the given @p packet.
    virtual void set(const packet::PacketPtr& packet) = 0;

    //! Get current stream position.
    //!
    //! Returns the position of the next sample to be retrieved by read(). Samples
    //! numbering depends on the specific protocol.
    //!
    //! A successfull read() increases the timestamp by the number of samples
    //! returned per channel. A set() call resets the timestamp according to the
    //! provided packet.
    virtual packet::timestamp_t timestamp() const = 0;

    //! Get number of samples remaining in the current packet.
    //!
    //! Returns zero if there are no more samples in the current packet of if there
    //! is no current packet set.
    //!
    //! A successfull read() decreases the number of remaining samples by the number
    //! of samples returned per channel. A set() call resets it according to the
    //! provided packet.
    virtual packet::timestamp_t remaining() const = 0;

    //! Decode samples.
    //!
    //! @b Parameters
    //!  - @p samples - buffer to write decoded samples to
    //!  - @p n_samples - number of samples to be decoded per channel
    //!  - @p channels - channel mask of the samples to be decoded
    //!
    //! Decodes samples from the current packet and writes them to the provided buffer.
    //!
    //! Packet channel mask and output samples channel mask may differ. If the packet
    //! provides additional channels, they are ignored. If the output samples mask
    //! don't have some channels present in packet, the corresponding channels in
    //! output buffer are set to zeros.
    //!
    //! @returns
    //!  number of samples decoded per channel. The returned value can be fewer than
    //!  @p n_samples if there are no more samples in the current packet.
    virtual size_t
    read(sample_t* samples, size_t n_samples, packet::channel_mask_t channels) = 0;

    //! Advance the stream position.
    //!
    //! @b Parameters
    //!  - @p m_samples - number of samples to add to the stream position
    //!
    //! Advances the stream position and drops the given number of leading samples
    //! (per channel), as if they were read and the result was droppped.
    //!
    //! The new position as allowed to go beyond the packet boundary. In this case,
    //! remaining() will report zero and read() will not read any samples, until
    //! set() is called.
    virtual void advance(size_t n_samples) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IDECODER_H_
