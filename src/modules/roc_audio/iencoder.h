/*
 * Copyright (c) 2017 Roc authors
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

    //! Calculate full packet size for given duration in nanoseconds.
    virtual size_t packet_size(core::nanoseconds_t duration) const = 0;

    //! Calculate packet payload size for given number of samples per channel.
    virtual size_t payload_size(size_t num_samples) const = 0;

    //! Start encoding a new packet.
    //!
    //! After this call, encoder will write samples to the given @p packet until
    //! it is full or end() is called.
    virtual void begin(const packet::PacketPtr& packet) = 0;

    //! Encode samples.
    //!
    //! @b Parameters
    //!  - @p samples - samples to be encoded
    //!  - @p n_samples - number of samples to be encoded per channel
    //!  - @p channels - channel mask of the samples to be encoded
    //!
    //! Encodes samples and writes to the current packet.
    //!
    //! Packet channel mask and input samples channel mask may differ. If the input
    //! samples provide additional channels, they are ignored. If the input samples
    //! don't have some channels present in packet, the corresponding channels in
    //! packet are set to zeros.
    //!
    //! @returns
    //!  number of samples encoded per channel. The returned value can be fewer than
    //!  @p n_samples if the packet is full and no more samples can be written to it.
    virtual size_t
    write(const sample_t* samples, size_t n_samples, packet::channel_mask_t channels) = 0;

    //! Finish encoding packet.
    //!
    //! If the packet is not fully filled with samples, the packet is padded.
    //!
    //! After this call, the packet is fully encoded and no more samples will be
    //! written to the packet. A new packet should be started by calling begin().
    virtual void end() = 0;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_IENCODER_H_
