/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
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

    //! Read samples from packet.
    //!
    //! @b Parameters
    //!  - @p packet - packet to read samples from
    //!  - @p offset - packet read offset
    //!  - @p samples - output buffer
    //!  - @p n_samples - number of samples in output buffer
    //!  - @p channels - output buffer channel mask
    //!
    //! Reads samples from @p packet starting from the @p offset and writes them to
    //! @p samples in the interleaved format, but no more than @p n_samples samples.
    //! The output buffer size should be at least n_samples * n_channels.
    //!
    //! Packet channel mask and output buffer channel mask may differ. If the packet
    //! contains additional channels, they are skipped. If the packet doesn't contain
    //! some requested channels, zero samples are inserted.
    //!
    //! @returns actual number of samples read for every channel.
    virtual size_t read_samples(const packet::Packet& packet,
                                size_t offset,
                                sample_t* samples,
                                size_t n_samples,
                                packet::channel_mask_t channels) = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IDECODER_H_
