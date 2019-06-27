/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_encoder.h
//! @brief PCM encoder.

#ifndef ROC_AUDIO_PCM_ENCODER_H_
#define ROC_AUDIO_PCM_ENCODER_H_

#include "roc_audio/iframe_encoder.h"
#include "roc_audio/pcm_funcs.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! PCM encoder.
class PCMEncoder : public IFrameEncoder, public core::NonCopyable<> {
public:
    //! Initialize.
    PCMEncoder(const PCMFuncs& funcs);

    //! Get packet payload size.
    virtual size_t payload_size(size_t num_samples) const;

    //! Write samples to packet.
    virtual size_t write_samples(packet::Packet& packet,
                                 size_t offset,
                                 const sample_t* samples,
                                 size_t n_samples,
                                 packet::channel_mask_t channels);

private:
    const PCMFuncs& funcs_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_ENCODER_H_
