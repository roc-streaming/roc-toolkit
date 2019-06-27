/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_decoder.h
//! @brief PCM decoder.

#ifndef ROC_AUDIO_PCM_DECODER_H_
#define ROC_AUDIO_PCM_DECODER_H_

#include "roc_audio/iframe_decoder.h"
#include "roc_audio/pcm_funcs.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! PCM decoder.
class PCMDecoder : public IFrameDecoder, public core::NonCopyable<> {
public:
    //! Initialize.
    PCMDecoder(const PCMFuncs& funcs);

    //! Read samples from packet.
    virtual size_t read_samples(const packet::Packet& packet,
                                size_t offset,
                                sample_t* samples,
                                size_t n_samples,
                                packet::channel_mask_t channels);

private:
    const PCMFuncs& funcs_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_DECODER_H_
