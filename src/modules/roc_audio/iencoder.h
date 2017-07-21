/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/iencoder.h
//! @brief Audio encoder interface.

#ifndef ROC_AUDIO_IENCODER_H_
#define ROC_AUDIO_IENCODER_H_

#include "roc_audio/units.h"
#include "roc_core/stddefs.h"
#include "roc_packet/packet.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Audio encoder interface.
class IEncoder {
public:
    virtual ~IEncoder();

    //! Get packet payload size.
    virtual size_t payload_size(size_t num_samples) const = 0;

    //! Write samples to packet.
    //!
    //! @b Parameters
    //!  - @p packet - packet to write samples to
    //!  - @p offset - packet write offset
    //!  - @p samples - input buffer
    //!  - @p n_samples - number of samples in input buffer
    //!  - @p channels - input buffer channel mask
    //!
    //! Reads @p n_samples from @p samples in the interleaved format, and writes
    //! them to @p packet starting from the @p offset. The input buffer size
    //! should be n_samples * n_channels.
    //!
    //! Packet channel mask and input buffer channel mask may differ. If the input
    //! buffer contains additional channels, they are skipped. If the input buffer
    //! doesn't contain some required channels, the corresponding packet channels
    //! are not modified.
    //!
    //! @returns actual number of samples written for every channel.
    virtual size_t write_samples(packet::Packet& packet,
                                 size_t offset,
                                 const sample_t* samples,
                                 size_t n_samples,
                                 packet::channel_mask_t channels) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IENCODER_H_
