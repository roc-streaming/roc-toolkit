/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/target_libvorbis/roc_audio/vorbis_encoder.h
//! @brief Vorbis audio encoder.

#ifndef ROC_AUDIO_VORBIS_ENCODER_H_
#define ROC_AUDIO_VORBIS_ENCODER_H_

#include "roc_audio/iframe_encoder.h"
#include "roc_audio/sample_spec.h"
#include <vorbis/codec.h>
#include <vorbis/vorbisenc.h>

namespace roc {
namespace audio {

//! Vorbis Encoder.
class VorbisEncoder : public IFrameEncoder {
public:
    //! Initialize.
    VorbisEncoder(const SampleSpec& sample_spec, core::IArena& arena);

    //! End.
    ~VorbisEncoder();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Get encoded frame size in bytes for given number of samples per channel.
    virtual size_t encoded_byte_count(size_t n_samples) const;

    //! Start encoding a new frame.
    virtual void begin_frame(void* frame, size_t frame_size);

    //! Encode samples.
    virtual size_t write_samples(const sample_t* samples, size_t n_samples);

    //! Finish encoding frame.
    virtual void end_frame();

private:
    void buffer_samples_(const sample_t* samples, size_t n_samples);
    void process_encoding_();

    bool initialized_;
    uint8_t* frame_data_;
    size_t frame_size_;
    size_t current_position_;
    vorbis_info vorbis_info_;
    vorbis_dsp_state vorbis_dsp_;
    vorbis_block vorbis_block_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_VORBIS_ENCODER_H_
