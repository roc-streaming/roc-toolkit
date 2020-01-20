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
class PcmDecoder : public IFrameDecoder, public core::NonCopyable<> {
public:
    //! Initialize.
    explicit PcmDecoder(const PcmFuncs& funcs);

    //! Get current stream position.
    virtual packet::timestamp_t position() const;

    //! Get number of samples available for decoding.
    virtual packet::timestamp_t available() const;

    //! Start decoding a new frame.
    virtual void
    begin(packet::timestamp_t frame_position, const void* frame_data, size_t frame_size);

    //! Read samples from current frame.
    virtual size_t
    read(sample_t* samples, size_t n_samples, packet::channel_mask_t channels);

    //! Shift samples from current frame.
    virtual size_t shift(size_t n_samples);

    //! Finish decoding current frame.
    virtual void end();

private:
    const PcmFuncs& funcs_;

    packet::timestamp_t stream_pos_;
    packet::timestamp_t stream_avail_;

    const void* frame_data_;
    size_t frame_size_;
    size_t frame_pos_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_DECODER_H_
