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
class PcmEncoder : public IFrameEncoder, public core::NonCopyable<> {
public:
    //! Initialize.
    explicit PcmEncoder(const PcmFuncs& funcs);

    //! Calculate encoded frame size for given number of samples per channel.
    virtual size_t encoded_size(size_t num_samples) const;

    //! Start encoding a new frame.
    virtual void begin(void* frame, size_t frame_size);

    //! Encode samples.
    virtual size_t
    write(const sample_t* samples, size_t n_samples, packet::channel_mask_t channels);

    //! Finish encoding frame.
    virtual void end();

private:
    const PcmFuncs& funcs_;

    void* frame_data_;
    size_t frame_size_;
    size_t frame_pos_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_ENCODER_H_
