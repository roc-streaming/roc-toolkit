/*
 * Copyright (c) 2015 Roc Streaming authors
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
#include "roc_audio/pcm_mapper.h"
#include "roc_audio/sample_spec.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! PCM decoder.
class PcmDecoder : public IFrameDecoder, public core::NonCopyable<> {
public:
    //! Construction function.
    static IFrameDecoder* construct(const SampleSpec& sample_spec, core::IArena& arena);

    //! Initialize.
    PcmDecoder(const SampleSpec& sample_spec, core::IArena& arena);

    //! Check if the object was successfully constructed.
    virtual status::StatusCode init_status() const;

    //! Get the header information before start decoding
    virtual bool initialize_headers(const uint8_t* headers, size_t headers_size);

    //! Get current stream position.
    virtual packet::stream_timestamp_t position() const;

    //! Get number of samples available for decoding.
    virtual packet::stream_timestamp_t available() const;

    //! Get number of samples per channel, that can be decoded from given frame.
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
    PcmMapper pcm_mapper_;
    const size_t n_chans_;

    packet::stream_timestamp_t stream_pos_;
    packet::stream_timestamp_t stream_avail_;

    const void* frame_data_;
    size_t frame_byte_size_;
    size_t frame_bit_off_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_DECODER_H_
