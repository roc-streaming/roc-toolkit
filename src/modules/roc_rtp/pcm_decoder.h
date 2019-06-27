/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/pcm_decoder.h
//! @brief PCM decoder.

#ifndef ROC_RTP_PCM_DECODER_H_
#define ROC_RTP_PCM_DECODER_H_

#include "roc_audio/iframe_decoder.h"
#include "roc_core/panic.h"
#include "roc_rtp/pcm_funcs.h"

namespace roc {
namespace rtp {

//! PCM decoder.
class PCMDecoder : public audio::IFrameDecoder, public core::NonCopyable<> {
public:
    //! Initialize.
    PCMDecoder(const PCMFuncs& funcs);

    //! Read samples from packet.
    virtual size_t read_samples(const packet::Packet& packet,
                                size_t offset,
                                audio::sample_t* samples,
                                size_t n_samples,
                                packet::channel_mask_t channels);

private:
    const PCMFuncs& funcs_;
};

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PCM_DECODER_H_
