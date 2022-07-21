/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/format_map.h"
#include "roc_audio/pcm_decoder.h"
#include "roc_audio/pcm_encoder.h"
#include "roc_core/panic.h"

namespace roc {
namespace rtp {

namespace {

template <audio::PcmEncoding Encoding,
          audio::PcmEndian Endian,
          size_t SampleRate,
          packet::channel_mask_t ChMask>
audio::IFrameEncoder* new_encoder(core::IAllocator& allocator) {
    return new (allocator) audio::PcmEncoder(audio::PcmFormat(Encoding, Endian),
                                             audio::SampleSpec(SampleRate, ChMask));
}

template <audio::PcmEncoding Encoding,
          audio::PcmEndian Endian,
          size_t SampleRate,
          packet::channel_mask_t ChMask>
audio::IFrameDecoder* new_decoder(core::IAllocator& allocator) {
    return new (allocator) audio::PcmDecoder(audio::PcmFormat(Encoding, Endian),
                                             audio::SampleSpec(SampleRate, ChMask));
}

} // namespace

FormatMap::FormatMap()
    : n_formats_(0) {
    {
        Format fmt;
        fmt.payload_type = PayloadType_L16_Mono;
        fmt.pcm_format =
            audio::PcmFormat(audio::PcmEncoding_SInt16, audio::PcmEndian_Big);
        fmt.sample_spec = audio::SampleSpec(44100, 0x1);
        fmt.packet_flags = packet::Packet::FlagAudio;
        fmt.new_encoder =
            &new_encoder<audio::PcmEncoding_SInt16, audio::PcmEndian_Big, 44100, 0x1>;
        fmt.new_decoder =
            &new_decoder<audio::PcmEncoding_SInt16, audio::PcmEndian_Big, 44100, 0x1>;
        add_(fmt);
    }
    {
        Format fmt;
        fmt.payload_type = PayloadType_L16_Stereo;
        fmt.pcm_format =
            audio::PcmFormat(audio::PcmEncoding_SInt16, audio::PcmEndian_Big);
        fmt.sample_spec = audio::SampleSpec(44100, 0x3);
        fmt.packet_flags = packet::Packet::FlagAudio;
        fmt.new_encoder =
            &new_encoder<audio::PcmEncoding_SInt16, audio::PcmEndian_Big, 44100, 0x3>;
        fmt.new_decoder =
            &new_decoder<audio::PcmEncoding_SInt16, audio::PcmEndian_Big, 44100, 0x3>;
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
