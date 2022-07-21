/*
 * Copyright (c) 2015 Roc Streaming authors
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
#include "roc_audio/pcm_format.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/iallocator.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

//! RTP payload format.
struct Format {
    //! Payload type.
    PayloadType payload_type;

    //! Sample encoding and endian.
    audio::PcmFormat pcm_format;

    //! Sample rate and channel mask.
    audio::SampleSpec sample_spec;

    //! Packet flags.
    unsigned packet_flags;

    //! Create frame encoder.
    audio::IFrameEncoder* (*new_encoder)(core::IAllocator& allocator);

    //! Create frame decoder.
    audio::IFrameDecoder* (*new_decoder)(core::IAllocator& allocator);

    //! Initialize.
    Format()
        : payload_type()
        , packet_flags()
        , new_encoder()
        , new_decoder() {
    }
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_FORMAT_H_
