/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_rtp/rtp_audio_format.h"
#include "roc_rtp/rtp_header.h"

namespace roc {
namespace rtp {

extern const RTP_AudioFormat RTP_AudioFormat_L16_Stereo;
extern const RTP_AudioFormat RTP_AudioFormat_L16_Mono;

const RTP_AudioFormat* get_audio_format_pt(uint8_t pt) {
    switch (pt) {
    case RTP_PT_L16_STEREO:
        return &RTP_AudioFormat_L16_Stereo;

    case RTP_PT_L16_MONO:
        return &RTP_AudioFormat_L16_Mono;

    default:
        break;
    }

    return NULL;
}

const RTP_AudioFormat* get_audio_format_cr(packet::channel_mask_t ch, size_t rate) {
    switch (ch) {
    case 0x1:
        if (RTP_AudioFormat_L16_Mono.rate == rate) {
            return &RTP_AudioFormat_L16_Mono;
        }

    case 0x3:
        if (RTP_AudioFormat_L16_Stereo.rate == rate) {
            return &RTP_AudioFormat_L16_Stereo;
        }

    default:
        break;
    }

    return NULL;
}

} // namespace rtp
} // namespace roc
