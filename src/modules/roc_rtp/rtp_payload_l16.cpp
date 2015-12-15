/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/stddefs.h"
#include "roc_core/panic.h"
#include "roc_core/log.h"
#include "roc_rtp/rtp_audio_format.h"

namespace roc {
namespace rtp {

namespace {

using namespace packet;

template <class Sample, size_t NumCh> size_t L16_n_samples(size_t payload_size) {
    return payload_size / NumCh / sizeof(Sample);
}

template <class Sample, size_t NumCh> size_t L16_size(size_t n_samples) {
    return n_samples * NumCh * sizeof(Sample);
}

template <class Sample, size_t NumCh, size_t MaxVal>
void L16_read(const void* payload,
              size_t offset,
              channel_mask_t ch_mask,
              sample_t* samples,
              size_t n_samples) {
    roc_panic_if_not(payload);
    roc_panic_if_not(samples);

    const Sample* pkt_samples = (const Sample*)payload + (offset * NumCh);

    for (size_t ns = 0; ns < n_samples; ns++) {
        channel_mask_t mask = ch_mask;

        for (size_t ch = 0; mask; ch++, mask >>= 1) {
            if (mask & 1) {
                switch (ch) {
                case 0:
                case 1:
                    *samples = sample_t(pkt_samples[ch % NumCh]) / MaxVal;
                    break;

                default:
                    *samples = 0;
                    break;
                }

                samples++;
            }
        }

        pkt_samples += NumCh;
    }
}

template <class Sample, size_t NumCh, size_t MaxVal>
void L16_write(void* payload,
               size_t offset,
               channel_mask_t ch_mask,
               const sample_t* samples,
               size_t n_samples) {
    roc_panic_if_not(payload);
    roc_panic_if_not(samples);

    Sample* pkt_samples = (Sample*)payload + (offset * NumCh);

    for (size_t ns = 0; ns < n_samples; ns++) {
        channel_mask_t mask = ch_mask;

        for (size_t ch = 0; mask; ch++, mask >>= 1) {
            if (mask & 1) {
                if (ch < NumCh) {
                    pkt_samples[ch] = Sample(*samples * MaxVal);
                }

                samples++;
            }
        }

        pkt_samples += NumCh;
    }
}

template <class Sample, size_t NumCh> void L16_clear(void* payload, size_t n_samples) {
    roc_panic_if_not(payload);

    memset(payload, 0, L16_size<Sample, NumCh>(n_samples));
}

} // namespace

extern const RTP_AudioFormat RTP_AudioFormat_L16_Stereo;

const RTP_AudioFormat RTP_AudioFormat_L16_Stereo = { RTP_PT_L16_STEREO,
                                                     0x3,
                                                     L16_n_samples<int16_t, 2>,
                                                     L16_size<int16_t, 2>,
                                                     L16_read<int16_t, 2, (1 << 15)>,
                                                     L16_write<int16_t, 2, (1 << 15)>,
                                                     L16_clear<int16_t, 2> };

extern const RTP_AudioFormat RTP_AudioFormat_L16_Mono;

const RTP_AudioFormat RTP_AudioFormat_L16_Mono = { RTP_PT_L16_MONO,
                                                   0x1,
                                                   L16_n_samples<int16_t, 1>,
                                                   L16_size<int16_t, 1>,
                                                   L16_read<int16_t, 1, (1 << 15)>,
                                                   L16_write<int16_t, 1, (1 << 15)>,
                                                   L16_clear<int16_t, 1> };

} // namespace rtp
} // namespace roc
