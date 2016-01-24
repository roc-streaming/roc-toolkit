/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/audio_format.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

extern const AudioFormat AudioFormat_L16_Stereo;
extern const AudioFormat AudioFormat_L16_Mono;

const AudioFormat* get_audio_format_pt(uint8_t pt) {
    switch (pt) {
    case RTP_PT_L16_STEREO:
        return &AudioFormat_L16_Stereo;

    case RTP_PT_L16_MONO:
        return &AudioFormat_L16_Mono;

    default:
        break;
    }

    return NULL;
}

const AudioFormat* get_audio_format_cr(packet::channel_mask_t ch, size_t rate) {
    switch (ch) {
    case 0x1:
        if (AudioFormat_L16_Mono.rate == rate) {
            return &AudioFormat_L16_Mono;
        }

    case 0x3:
        if (AudioFormat_L16_Stereo.rate == rate) {
            return &AudioFormat_L16_Stereo;
        }

    default:
        break;
    }

    return NULL;
}

} // namespace rtp
} // namespace roc
