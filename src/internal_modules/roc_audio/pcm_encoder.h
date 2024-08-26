/*
 * Copyright (c) 2015 Roc Streaming authors
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
#include "roc_audio/pcm_mapper.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! PCM encoder.
class PcmEncoder : public IFrameEncoder, public core::NonCopyable<> {
public:
    //! Construction function.
    static IFrameEncoder* construct(const SampleSpec& sample_spec, core::IArena& arena);

    //! Initialize.
    PcmEncoder(const SampleSpec& sample_spec, core::IArena& arena);

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Get encoded frame size in bytes for given number of samples per channel.
    virtual size_t encoded_byte_count(size_t num_samples) const;

    //! Get headers frame.
    const uint8_t* get_headers_frame() const;

    //! Get the size of the headers.
    size_t get_headers_frame_size() const;

    //! Start encoding a new frame.
    virtual void begin_frame(void* frame, size_t frame_size);

    //! Encode samples.
    virtual size_t write_samples(const sample_t* samples, size_t n_samples);

    //! Finish encoding frame.
    virtual void end_frame();

private:
    PcmMapper pcm_mapper_;
    const size_t n_chans_;

    void* frame_data_;
    size_t frame_byte_size_;
    size_t frame_bit_off_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_ENCODER_H_
