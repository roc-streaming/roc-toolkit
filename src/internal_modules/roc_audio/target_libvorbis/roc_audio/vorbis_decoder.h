/*
 * Copyright (c) 2024 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/target_libvorbis/roc_audio/vorbis_decoder.h
//! @brief Vorbis audio decoder.

#ifndef ROC_AUDIO_VORBIS_DECODER_H_
#define ROC_AUDIO_VORBIS_DECODER_H_

#include "roc_audio/iframe_decoder.h"
#include "roc_audio/sample_spec.h"
#include <vorbis/vorbisfile.h>

namespace roc {
namespace audio {

//! Vorbis Decoder.
class VorbisDecoder : public IFrameDecoder {
public:
    //! Initialize.
    VorbisDecoder(const SampleSpec& sample_spec);

    //! End.
    ~VorbisDecoder();

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Get decoded stream position.
    virtual packet::stream_timestamp_t position() const;

    //! Get number of samples available for decoding.
    virtual packet::stream_timestamp_t available() const;

    //! Get number of samples per channel that can be decoded from given frame.
    virtual size_t decoded_sample_count(const void* frame_data, size_t frame_size) const;

    //! Start decoding a new frame.
    virtual void begin_frame(packet::stream_timestamp_t frame_position,
                             const void* frame_data,
                             size_t frame_size);

    //! Read samples from current frame.
    virtual size_t read_samples(sample_t* samples, size_t n_samples);

    //! Shift samples from current frame.
    virtual size_t drop_samples(size_t n_samples);

    //! Finish decoding current frame.
    virtual void end_frame();

private:
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_VORBIS_DECODER_H_
