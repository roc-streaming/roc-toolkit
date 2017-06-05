/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_core/byte_order.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"
#include "roc_core/stddefs.h"
#include "roc_rtp/audio_format.h"

namespace roc {
namespace rtp {

using namespace packet;

namespace {

sample_t pcm_unpack(int16_t ns) {
    const int16_t hs = (int16_t)ROC_NTOH_16(uint16_t(ns));

    return sample_t(hs) / (1 << 15);
}

template <class T> T pcm_pack(sample_t);

template <> int16_t pcm_pack(sample_t fs) {
    const int16_t hs = int16_t(fs * (1 << 15));

    return (int16_t)ROC_HTON_16(uint16_t(hs));
}

template <class Sample, size_t NumCh> size_t pcm_n_samples(size_t payload_size) {
    return payload_size / NumCh / sizeof(Sample);
}

template <class Sample, size_t NumCh> size_t pcm_size(size_t n_samples) {
    return n_samples * NumCh * sizeof(Sample);
}

template <class Sample, size_t NumCh>
void pcm_read(const void* payload,
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
                    *samples = pcm_unpack(pkt_samples[ch % NumCh]);
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

template <class Sample, size_t NumCh>
void pcm_write(void* payload,
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
                    pkt_samples[ch] = pcm_pack<Sample>(*samples);
                }

                samples++;
            }
        }

        pkt_samples += NumCh;
    }
}

template <class Sample, size_t NumCh> void pcm_clear(void* payload, size_t n_samples) {
    roc_panic_if_not(payload);

    memset(payload, 0, pcm_size<Sample, NumCh>(n_samples));
}

} // namespace

extern const AudioFormat AudioFormat_L16_Stereo;

const AudioFormat AudioFormat_L16_Stereo = {
    //
    RTP_PT_L16_STEREO,         //
    0x3,                       //
    44100,                     //
    pcm_n_samples<int16_t, 2>, //
    pcm_size<int16_t, 2>,      //
    pcm_read<int16_t, 2>,      //
    pcm_write<int16_t, 2>,     //
    pcm_clear<int16_t, 2>      //
};

extern const AudioFormat AudioFormat_L16_Mono;

const AudioFormat AudioFormat_L16_Mono = {
    //
    RTP_PT_L16_MONO,           //
    0x1,                       //
    44100,                     //
    pcm_n_samples<int16_t, 1>, //
    pcm_size<int16_t, 1>,      //
    pcm_read<int16_t, 1>,      //
    pcm_write<int16_t, 1>,     //
    pcm_clear<int16_t, 1>      //
};

} // namespace rtp
} // namespace roc
