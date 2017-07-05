/*
 * Copyright (c) 2017 Mikhail Baranov
 * Copyright (c) 2017 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/format_map.h"
#include "roc_rtp/pcm_decoder.h"
#include "roc_rtp/pcm_encoder.h"
#include "roc_rtp/pcm_helpers.h"

namespace roc {
namespace rtp {

namespace {

Format pcm_l16_stereo = {
    /* payload_type */ PayloadType_L16_Stereo,
    /* flags        */ packet::Packet::FlagAudio,
    /* sample_rate  */ 44100,
    /* channel_mask */ 0x3,
    /* duration     */ &pcm_duration<int16_t, 2>,
    /* size         */ &pcm_packet_size<int16_t, 2>,
    /* new_encoder  */ &PCMEncoder<int16_t, 2>::create,
    /* new_decoder  */ &PCMDecoder<int16_t, 2>::create,
};

Format pcm_l16_mono = {
    /* payload_type */ PayloadType_L16_Mono,
    /* flags        */ packet::Packet::FlagAudio,
    /* sample_rate  */ 44100,
    /* channel_mask */ 0x1,
    /* duration     */ &pcm_duration<int16_t, 1>,
    /* size         */ &pcm_packet_size<int16_t, 1>,
    /* new_encoder  */ &PCMEncoder<int16_t, 1>::create,
    /* new_decoder  */ &PCMDecoder<int16_t, 1>::create,
};

} // namespace

const Format* FormatMap::format(unsigned int pt) const {
    switch (pt) {
    case PayloadType_L16_Stereo:
        return &pcm_l16_stereo;

    case PayloadType_L16_Mono:
        return &pcm_l16_mono;

    default:
        return NULL;
    }
}

} // namespace rtp
} // namespace roc
