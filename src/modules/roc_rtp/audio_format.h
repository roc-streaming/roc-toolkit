/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/audio_format.h
//! @brief RTP audio format.

#ifndef ROC_RTP_AUDIO_FORMAT_H_
#define ROC_RTP_AUDIO_FORMAT_H_

#include "roc_core/stddefs.h"
#include "roc_packet/units.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

//! RTP audio format.
struct AudioFormat {
    //! Payload type.
    RTP_PayloadType pt;

    //! Bitmask of supported channels.
    packet::channel_mask_t channels;

    //! Sample rate.
    size_t rate;

    //! Get number of samples in packet.
    size_t (*n_samples)(size_t payload_size);

    //! Get number of samples in packet.
    size_t (*size)(size_t n_samples);

    //! Read samples from payload.
    void (*read)(const void* payload,
                 size_t offset,
                 packet::channel_mask_t ch_mask,
                 packet::sample_t* samples,
                 size_t n_samples);

    //! Write samples to payload.
    void (*write)(void* payload,
                  size_t offset,
                  packet::channel_mask_t ch_mask,
                  const packet::sample_t* samples,
                  size_t n_samples);

    //! Clear payload.
    void (*clear)(void* payload, size_t n_samples);
};

//! Get audio format from payload type.
const AudioFormat* get_audio_format_pt(uint8_t pt);

//! Get audio format from channel mask and sample rate.
const AudioFormat* get_audio_format_cr(packet::channel_mask_t ch, size_t rate);

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_AUDIO_FORMAT_H_
