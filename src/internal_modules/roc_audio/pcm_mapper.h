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

#include "roc_audio/pcm_subformat.h"
#include "roc_core/noncopyable.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace audio {

//! PCM format mapper.
//! Converts between two PCM formats.
//! Either input or output format must be raw samples (PcmSubformat_Raw).
class PcmMapper : public core::NonCopyable<> {
public:
    //! Initialize.
    PcmMapper(PcmSubformat input_fmt, PcmSubformat output_fmt);

    //! Get input format.
    PcmSubformat input_format() const;

    //! Get output format.
    PcmSubformat output_format() const;

    //! Get number of input samples (total for all channels) for given number of bytes.
    size_t input_sample_count(size_t input_bytes) const;

    //! Get number of input samples (total for all channels) for given number of bytes.
    size_t output_sample_count(size_t output_bytes) const;

    //! Get number of input bytes for given number of samples (total for all channels).
    size_t input_byte_count(size_t input_samples) const;

    //! Get number of output bytes for given number of samples (total for all channels).
    size_t output_byte_count(size_t output_samples) const;

    //! Get number of input bits for given number of samples (total for all channels).
    size_t input_bit_count(size_t input_samples) const;

    //! Get number of output bits for given number of samples (total for all channels).
    size_t output_bit_count(size_t output_samples) const;

    //! Map samples from input to output format.
    //! @remarks
    //!  @p in_data is a pointer to input buffer
    //!  @p in_byte_size is size of input buffer in bytes
    //!  @p in_bit_off is an offset in input buffer in bits
    //!  @p out_data is a pointer to output buffer
    //!  @p out_byte_size is size of output buffer in bytes
    //!  @p out_bit_off is an offset in output buffer in bits
    //!  @p n_samples is number of input and output samples
    //!  (total for all channels) to be mapped
    //! @returns
    //!  number of samples actually mapped, which may be truncated if
    //!  input or output buffer is smaller than requested
    //! @note
    //!  increments @p in_bit_off and @p out_bit_off by the number
    //!  of mapped bits
    size_t map(const void* in_data,
               size_t in_byte_size,
               size_t& in_bit_off,
               void* out_data,
               size_t out_byte_size,
               size_t& out_bit_off,
               size_t n_samples);

private:
    const PcmSubformat input_fmt_;
    const PcmSubformat output_fmt_;

    const PcmTraits input_traits_;
    const PcmTraits output_traits_;

    PcmMapFn map_func_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_MAPPER_H_
