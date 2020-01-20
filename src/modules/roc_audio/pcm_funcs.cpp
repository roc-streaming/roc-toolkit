/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/pcm_funcs.h"
#include "roc_core/endian.h"

namespace roc {
namespace audio {

namespace {

template <class Sample, size_t NumCh>
size_t pcm_samples_from_payload_size(size_t payload_size) {
    return payload_size / NumCh / sizeof(Sample);
}

template <class Sample, size_t NumCh>
size_t pcm_payload_size_from_samples(size_t num_samples) {
    return num_samples * NumCh * sizeof(Sample);
}

template <class T> T pcm_encode_one_sample(sample_t);

template <> int16_t inline pcm_encode_one_sample(float s) {
    s *= 32768.0f;
    s = std::min(s, +32767.0f);
    s = std::max(s, -32768.0f);
    return (int16_t)core::hton16((uint16_t)(int16_t)s);
}

inline float pcm_decode_one_sample(int16_t s) {
    return float((int16_t)core::ntoh16((uint16_t)s)) / 32768.0f;
}

template <class Sample, size_t NumCh>
size_t pcm_encode_samples(void* out_data,
                          size_t out_size,
                          size_t out_offset,
                          const sample_t* in_samples,
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
                    *out_samples++ = pcm_encode_one_sample<Sample>(*in_samples);
                }
                in_samples++;
            } else {
                if (out_chan_mask & ch) {
                    *out_samples++ = 0;
                }
            }
        }
    }

    return in_n_samples;
}

template <class Sample, size_t NumCh>
size_t pcm_decode_samples(const void* in_data,
                          size_t in_size,
                          size_t in_offset,
                          sample_t* out_samples,
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
            sample_t s = 0;
            if (in_chan_mask & ch) {
                s = pcm_decode_one_sample(*in_samples++);
            }
            if (out_chan_mask & ch) {
                *out_samples++ = s;
            }
        }
    }

    return out_n_samples;
}

} // namespace

const PcmFuncs PCM_int16_1ch = {
    pcm_samples_from_payload_size<int16_t, 1>,
    pcm_payload_size_from_samples<int16_t, 1>,
    pcm_encode_samples<int16_t, 1>,
    pcm_decode_samples<int16_t, 1>,
};

const PcmFuncs PCM_int16_2ch = {
    pcm_samples_from_payload_size<int16_t, 2>,
    pcm_payload_size_from_samples<int16_t, 2>,
    pcm_encode_samples<int16_t, 2>,
    pcm_decode_samples<int16_t, 2>,
};

} // namespace audio
} // namespace roc
