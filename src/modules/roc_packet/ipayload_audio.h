/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/ipayload_audio.h
//! @brief Audio payload interface.

#ifndef ROC_PACKET_IPAYLOAD_AUDIO_H_
#define ROC_PACKET_IPAYLOAD_AUDIO_H_

#include "roc_packet/units.h"

namespace roc {
namespace packet {

//! Audio payload interface.
class IPayloadAudio {
public:
    virtual ~IPayloadAudio();

    //! Get bitmask of channels present in packet.
    virtual channel_mask_t channels() const = 0;

    //! Get number of samples in packet.
    virtual size_t num_samples() const = 0;

    //! Set channel mask, number of samples per channel and sample rate.
    virtual void configure(channel_mask_t ch_mask, size_t n_samples, size_t rate) = 0;

    //! Read samples from packet.
    //!
    //! @returns actual number of samples per channel that was copied.
    //!
    //! @remarks
    //!  Copy min(n_samples, num_samples) * n_channels samples from packet's
    //!  buffer to @p samples.
    //!
    //! @p ch_mask specifies bitmask of channels present in @p samples. n_channels
    //! is calculated from @p ch_mask.
    //!
    //! @p offset specifies offset inside packet's buffer in samples (not in bytes).
    //!
    //! @note
    //!  Can be called multiple times to write samples for different channels and
    //!  offsets.
    virtual size_t read_samples(channel_mask_t ch_mask,
                                size_t offset,
                                sample_t* samples,
                                size_t n_samples) const = 0;

    //! Write samples to packet.
    //!
    //! @remarks
    //!  Copy (n_samples * n_channels) samples in interleaved format from
    //!  @p samples to packet's buffer.
    //!
    //! @p ch_mask specifies bitmask of channels present in @p samples. n_channels
    //! is calculated from @p ch_mask.
    //!
    //! @p offset specifies offset inside packet's buffer in samples (not in bytes).
    //!
    //! @note
    //!  Can be called multiple times to write samples for different channels and
    //!  offsets.
    //!
    //! @pre
    //!  set_size() should be called.
    virtual void write_samples(channel_mask_t ch_mask,
                               size_t offset,
                               const sample_t* samples,
                               size_t n_samples) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IPAYLOAD_AUDIO_H_
