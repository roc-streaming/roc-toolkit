/*
 * Copyright (c) 2015 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/encoding.h
//! @brief RTP encoding.

#ifndef ROC_RTP_ENCODING_H_
#define ROC_RTP_ENCODING_H_

#include "roc_audio/iframe_decoder.h"
#include "roc_audio/iframe_encoder.h"
#include "roc_audio/pcm_format.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/iarena.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

//! RTP encoding.
struct Encoding {
    //! Payload type.
    unsigned int payload_type;

    //! Encoding specification.
    audio::SampleSpec sample_spec;

    //! Packet flags.
    unsigned packet_flags;

    //! Create frame encoder.
    audio::IFrameEncoder* (*new_encoder)(core::IArena& arena,
                                         const audio::SampleSpec& sample_spec);

    //! Create frame decoder.
    audio::IFrameDecoder* (*new_decoder)(core::IArena& arena,
                                         const audio::SampleSpec& sample_spec);

    //! Initialize.
    Encoding()
        : payload_type(0)
        , sample_spec()
        , packet_flags(0)
        , new_encoder(NULL)
        , new_decoder(NULL) {
    }
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_ENCODING_H_
