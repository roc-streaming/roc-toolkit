/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/vorbis_encoder.h
//! @brief TODO.

#ifndef ROC_AUDIO_VORBIS_ENCODER_H_
#define ROC_AUDIO_VORBIS_ENCODER_H_

#include "roc_audio/iframe_encoder.h"
#include "roc_audio/sample_spec.h"
#include <vorbis/vorbisenc.h>

namespace roc {
namespace audio {

class VorbisEncoder : public IFrameEncoder {
public:
    VorbisEncoder(const SampleSpec& sample_spec);
    ~VorbisEncoder();

    virtual status::StatusCode init_status() const;

    virtual size_t encoded_byte_count(size_t n_samples) const;

    virtual void begin_frame(void* frame_data, size_t frame_size);

    virtual size_t write_samples(const sample_t* samples, size_t n_samples);

    virtual void end_frame();

private:
    SampleSpec sample_spec_;
    bool initialized_;
    void* frame_data_;
    size_t frame_size_;
    ogg_stream_state ogg_stream_;
    vorbis_info vorbis_info_;
    vorbis_comment vorbis_comment_;
    vorbis_dsp_state vorbis_dsp_;
    vorbis_block vorbis_block_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_VORBIS_ENCODER_H_
