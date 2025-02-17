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

//! PCM signed integers.
const uint16_t WAV_FORMAT_PCM = 0x0001;
//! PCM IEEE floats.
const uint16_t WAV_FORMAT_IEEE_FLOAT = 0x0003;

//! WAV header.
//! @remarks
//!  Holds data of a WAV header.
//!  Allows easy generation of WAV header.
//!  Reference:
//!  https://www-mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html
class WavHeader {
public:
    //! WAV header data
    ROC_ATTR_PACKED_BEGIN struct WavHeaderData {
        //! Constructor
        WavHeaderData(uint32_t chunk_id,
                      uint32_t chunk_size,
                      uint32_t form_type,
                      uint32_t subchunk1_id,
                      uint32_t subchunk1_size,
                      uint16_t audio_format,
                      uint16_t num_channels,
                      uint32_t sample_rate,
                      uint32_t byte_rate,
                      uint16_t block_align,
                      uint16_t bits_per_sample,
                      uint32_t subchunk2_id,
                      uint32_t subchunk2_size);
        // RIFF header
        //! Chunk ID
        const uint32_t chunk_id;
        //! Chunk size
        uint32_t chunk_size;
        //! Format
        const uint32_t form_type;

        // WAVE fmt
        //! Subchunk1 ID
        const uint32_t subchunk1_id;
        //! Subchunk1 size
        const uint32_t subchunk1_size;
        //! Audio format
        const uint16_t audio_format;
        //! Num channels
        const uint16_t num_channels;
        //! Sample rate
        const uint32_t sample_rate;
        //! Byte rate
        const uint32_t byte_rate;
        //! Block align
        const uint16_t block_align;
        //! Bits per sample
        const uint16_t bits_per_sample;

        // WAVE data
        //! Subchunk2 ID
        const uint32_t subchunk2_id;
        //! Subchunk2 size
        uint32_t subchunk2_size;
    } ROC_ATTR_PACKED_END;

    //! Initialize
    WavHeader(const uint16_t format_tag,
              const uint16_t bits_per_sample,
              const uint32_t sample_rate,
              const uint16_t num_channels);

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
