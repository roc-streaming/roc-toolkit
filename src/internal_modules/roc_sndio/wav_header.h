/*
 * Copyright (c) 2023 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_sndio/wav_header.h
//! @brief WAV header.

#ifndef ROC_SNDIO_WAV_HEADER_H_
#define ROC_SNDIO_WAV_HEADER_H_

#include "roc_core/attributes.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace sndio {

//! WAV header
//! @remarks
//!  Holds data of a WAV header
//!  Allows easy generation of WAV header
class WavHeader {
public:
    //! WAV header data
    ROC_ATTR_PACKED_BEGIN struct WavHeaderData {
        //! Constructor
        WavHeaderData(uint32_t chunk_id,
                      uint32_t format,
                      uint32_t subchunk1_id,
                      uint32_t subchunk1_size,
                      uint16_t audio_format,
                      uint16_t num_channels,
                      uint32_t sample_rate,
                      uint32_t byte_rate,
                      uint16_t block_align,
                      uint16_t bits_per_sample,
                      uint32_t subchunk2_id);
        // RIFF header
        //! Chunk ID
        const uint32_t chunk_id_;
        //! Chunk size
        uint32_t chunk_size_;
        //! Format
        const uint32_t format_;
        // WAVE fmt
        //! Subchunk1 ID
        const uint32_t subchunk1_id_;
        //! Subchunk1 size
        const uint32_t subchunk1_size_;
        //! Audio format
        const uint16_t audio_format_;
        //! Num channels
        const uint16_t num_channels_;
        //! Sample rate
        const uint32_t sample_rate_;
        //! Byte rate
        const uint32_t byte_rate_;
        //! Block align
        const uint16_t block_align_;
        //! Bits per sample
        const uint16_t bits_per_sample_;
        // WAVE data
        //! Subchunk2 ID
        const uint32_t subchunk2_id_;
        //! Subchunk2 size
        uint32_t subchunk2_size_;
    } ROC_ATTR_PACKED_END;

    //! Initialize
    WavHeader(uint16_t num_channels, uint32_t sample_rate, uint16_t bits_per_sample);

    //! Get number of channels
    uint16_t num_channels() const;

    //! Get sample rate
    uint32_t sample_rate() const;

    //! Get number of bits per sample
    uint16_t bits_per_sample() const;

    //! Resets samples counter
    void reset_sample_counter(uint32_t num_samples);

    //! Updates samples num and returns header data
    const WavHeaderData& update_and_get_header(uint32_t num_samples);

private:
    static const uint32_t METADATA_SIZE_ = 36;

    WavHeaderData data_;
    // Help data
    uint32_t num_samples_;
};

} // namespace sndio
} // namespace roc

#endif // ROC_SNDIO_WAV_HEADER_H_
