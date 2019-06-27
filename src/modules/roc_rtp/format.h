/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/format.h
//! @brief RTP payload format.

#ifndef ROC_RTP_FORMAT_H_
#define ROC_RTP_FORMAT_H_

#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_core/iallocator.h"
#include "roc_core/time.h"
#include "roc_packet/rtp.h"
#include "roc_packet/units.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

//! RTP payload format.
struct Format {
    //! Payload type.
    PayloadType payload_type;

    //! Packet flags.
    unsigned flags;

    //! Sample rate.
    size_t sample_rate;

    //! Channel mask.
    packet::channel_mask_t channel_mask;

    //! Get number of samples for given payload size.
    size_t (*get_num_samples)(size_t payload_size);

    //! Create encoder.
    audio::IFrameEncoder* (*new_encoder)(core::IAllocator& allocator);

    //! Create decoder.
    audio::IFrameDecoder* (*new_decoder)(core::IAllocator& allocator);
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_FORMAT_H_
