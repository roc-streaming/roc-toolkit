/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtp/pcm_helpers.h
//! @brief PCM helpers.

#ifndef ROC_RTP_PCM_HELPERS_H_
#define ROC_RTP_PCM_HELPERS_H_

#include "roc_audio/units.h"
#include "roc_core/endian.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"
#include "roc_rtp/headers.h"

namespace roc {
namespace rtp {

//! Calculate packet duration.
template <class Sample, size_t NumCh>
packet::timestamp_t pcm_duration(const packet::RTP& rtp) {
    return packet::timestamp_t(rtp.payload.size() / NumCh / sizeof(Sample));
}

//! Calculate payload size.
template <class Sample, size_t NumCh> size_t pcm_payload_size(size_t num_samples) {
    return num_samples * NumCh * sizeof(Sample);
}

//! Calculate packet size.
template <class Sample, size_t NumCh> size_t pcm_packet_size(size_t num_samples) {
    return sizeof(Header) + pcm_payload_size<Sample, NumCh>(num_samples);
}

//! Encode single sample.
template <class T> T pcm_pack(audio::sample_t);

//! Encode single sample (int16_t).
template <> int16_t inline pcm_pack(float s) {
    s *= 32768.0f;
    s = ROC_MIN(s, +32767.0f);
    s = ROC_MAX(s, -32768.0f);
    return (int16_t)ROC_HTON_16(int16_t(s));
}

//! Decode single sample (int16_t).
inline float pcm_unpack(int16_t s) {
    return float((int16_t)ROC_NTOH_16(uint16_t(s))) / 32768.0f;
}

//! Decode multiple samples.
template <class Sample, size_t NumCh>
size_t pcm_write(void* out_data,
                 size_t out_size,
                 size_t out_offset,
                 const audio::sample_t* in_samples,
                 size_t in_n_samples,
                 packet::channel_mask_t in_chan_mask) {
    const packet::channel_mask_t out_chan_mask = packet::channel_mask_t(1 << NumCh) - 1;
    const packet::channel_mask_t inout_chan_mask = in_chan_mask | out_chan_mask;

    size_t len = out_size / NumCh / sizeof(Sample);
    size_t off = out_offset;
    if (off > len) {
        off = len;
    }

    if (in_n_samples > (len - off)) {
        in_n_samples = (len - off);
    }

    Sample* out_samples = (Sample*)out_data + (off * NumCh);

    for (size_t ns = 0; ns < in_n_samples; ns++) {
        for (packet::channel_mask_t ch = 1; ch <= inout_chan_mask && ch != 0; ch <<= 1) {
            if (in_chan_mask & ch) {
                if (out_chan_mask & ch) {
                    *out_samples = pcm_pack<Sample>(*in_samples);
                }
                in_samples++;
            }
            if (out_chan_mask & ch) {
                out_samples++;
            }
        }
    }

    return in_n_samples;
}

//! Decode multiple samples.
template <class Sample, size_t NumCh>
size_t pcm_read(const void* in_data,
                size_t in_size,
                size_t in_offset,
                audio::sample_t* out_samples,
                size_t out_n_samples,
                packet::channel_mask_t out_chan_mask) {
    const packet::channel_mask_t in_chan_mask = packet::channel_mask_t(1 << NumCh) - 1;
    const packet::channel_mask_t inout_chan_mask = in_chan_mask | out_chan_mask;

    size_t len = in_size / NumCh / sizeof(Sample);
    size_t off = in_offset;
    if (off > len) {
        off = len;
    }

    if (out_n_samples > (len - off)) {
        out_n_samples = (len - off);
    }

    const Sample* in_samples = (const Sample*)in_data + (off * NumCh);

    for (size_t ns = 0; ns < out_n_samples; ns++) {
        for (packet::channel_mask_t ch = 1; ch <= inout_chan_mask && ch != 0; ch <<= 1) {
            audio::sample_t s = 0;
            if (in_chan_mask & ch) {
                s = pcm_unpack(*in_samples++);
            }
            if (out_chan_mask & ch) {
                *out_samples++ = s;
            }
        }
    }

    return out_n_samples;
}

} // namespace rtp
} // namespace roc

#endif // ROC_RTP_PCM_HELPERS_H_
