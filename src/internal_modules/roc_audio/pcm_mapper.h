/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_mapper.h
//! @brief PCM format mapper.

#ifndef ROC_AUDIO_PCM_MAPPER_H_
#define ROC_AUDIO_PCM_MAPPER_H_

#include "roc_audio/pcm_format.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! PCM format mapper.
//! Convert between PCM formats.
class PcmMapper : public core::NonCopyable<> {
public:
    //! Initialize.
    PcmMapper(const PcmFormat& input_fmt, const PcmFormat& output_fmt);

    //! Get input format.
    const PcmFormat& input_format() const;

    //! Get output format.
    const PcmFormat& output_format() const;

    //! Get number of input samples per channel for given number of bytes.
    size_t input_sample_count(size_t input_bytes) const;

    //! Get number of input samples per channel for given number of bytes.
    size_t output_sample_count(size_t output_bytes) const;

    //! Get number of input bytes for given number of samples per channel.
    size_t input_byte_count(size_t input_samples) const;

    //! Get number of output bytes for given number of samples per channel.
    size_t output_byte_count(size_t output_samples) const;

    //! Get number of input bits for given number of samples per channel.
    size_t input_bit_count(size_t input_samples) const;

    //! Get number of output bits for given number of samples per channel.
    size_t output_bit_count(size_t output_samples) const;

    //! Map samples from input to output format.
    //! @remarks
    //!  @p in_data is a pointer to input buffer
    //!  @p in_byte_size is size of input buffer in bytes
    //!  @p in_bit_off is an offset in input buffer in bits
    //!  @p out_data is a pointer to output buffer
    //!  @p out_byte_size is size of output buffer in bytes
    //!  @p out_bit_off is an offset in output buffer in bits
    //!  @p n_samples is number of input and output samples for all channels
    //! @returns
    //!  number of samples actually mapped, which may be truncated if
    //!  input or output buffer is smaller than requested
    //! @note
    //!  updates @p in_bit_off and @p out_bit_off
    size_t map(const void* in_data,
               size_t in_byte_size,
               size_t& in_bit_off,
               void* out_data,
               size_t out_byte_size,
               size_t& out_bit_off,
               size_t n_samples);

private:
    const PcmFormat input_fmt_;
    const PcmFormat output_fmt_;

    const size_t input_sample_bits_;
    const size_t output_sample_bits_;

    void (*const map_func_)(const uint8_t* in_data,
                            size_t& in_bit_off,
                            uint8_t* out_data,
                            size_t& out_bit_off,
                            size_t n_samples);
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_MAPPER_H_
