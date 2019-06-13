/*
 * Copyright (c) 2017 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/format_map.h"
#include "roc_core/panic.h"
#include "roc_rtp/pcm_decoder.h"
#include "roc_rtp/pcm_encoder.h"
#include "roc_rtp/pcm_funcs.h"

namespace roc {
namespace rtp {

namespace {

// PCM 16-bit 2ch

audio::IEncoder* new_encoder_pcm_16bit_2ch(core::IAllocator& allocator,
                                           const Format& format) {
    return new (allocator) PCMEncoder(PCM_16bit_2ch, format);
}

audio::IDecoder* new_decoder_pcm_16bit_2ch(core::IAllocator& allocator,
                                           const Format& format) {
    return new (allocator) PCMDecoder(PCM_16bit_2ch, format);
}

// PCM 16-bit 1ch

audio::IEncoder* new_encoder_pcm_16bit_1ch(core::IAllocator& allocator,
                                           const Format& format) {
    return new (allocator) PCMEncoder(PCM_16bit_1ch, format);
}

audio::IDecoder* new_decoder_pcm_16bit_1ch(core::IAllocator& allocator,
                                           const Format& format) {
    return new (allocator) PCMDecoder(PCM_16bit_1ch, format);
}

} // namespace

FormatMap::FormatMap()
    : n_formats_(0) {
    {
        Format fmt;
        fmt.payload_type = PayloadType_L16_Stereo;
        fmt.flags = packet::Packet::FlagAudio;
        fmt.sample_rate = 44100;
        fmt.channel_mask = 0x3;
        fmt.get_num_samples = PCM_16bit_2ch.samples_from_payload_size;
        fmt.new_encoder = new_encoder_pcm_16bit_2ch;
        fmt.new_decoder = new_decoder_pcm_16bit_2ch;
        add_(fmt);
    }
    {
        Format fmt;
        fmt.payload_type = PayloadType_L16_Mono;
        fmt.flags = packet::Packet::FlagAudio;
        fmt.sample_rate = 44100;
        fmt.channel_mask = 0x1;
        fmt.get_num_samples = PCM_16bit_1ch.samples_from_payload_size;
        fmt.new_encoder = new_encoder_pcm_16bit_1ch;
        fmt.new_decoder = new_decoder_pcm_16bit_1ch;
        add_(fmt);
    }
}

const Format* FormatMap::format(unsigned int pt) const {
    for (size_t n = 0; n < n_formats_; n++) {
        if ((unsigned int)formats_[n].payload_type == pt) {
            return &formats_[n];
        }
    }

    return NULL;
}

void FormatMap::add_(const Format& fmt) {
    roc_panic_if(n_formats_ == MaxFormats);
    formats_[n_formats_++] = fmt;
}

} // namespace rtp
} // namespace roc
