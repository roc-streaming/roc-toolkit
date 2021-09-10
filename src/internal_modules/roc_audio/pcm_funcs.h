/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/pcm_funcs.h
//! @brief RTP PCM functions.

#ifndef ROC_AUDIO_PCM_FUNCS_H_
#define ROC_AUDIO_PCM_FUNCS_H_

#include "roc_audio/units.h"
#include "roc_core/stddefs.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! PCM function table.
struct PcmFuncs {
    //! Get number of samples per channel from payload size in bytes.
    size_t (*samples_from_payload_size)(size_t payload_size);

    //! Get payload size in bytes from number of samples per channel.
    size_t (*payload_size_from_samples)(size_t num_samples);

    //! Encode samples.
    size_t (*encode_samples)(void* out_data,
                             size_t out_size,
                             size_t out_offset,
                             const sample_t* in_samples,
                             size_t in_n_samples,
                             packet::channel_mask_t in_chan_mask);

    //! Decode samples.
    size_t (*decode_samples)(const void* in_data,
                             size_t in_size,
                             size_t in_offset,
                             sample_t* out_samples,
                             size_t out_n_samples,
                             packet::channel_mask_t out_chan_mask);
};

//! PCM functions for 16-bit 1-channel audio.
extern const PcmFuncs PCM_int16_1ch;

//! PCM functions for 16-bit 2-channel audio.
extern const PcmFuncs PCM_int16_2ch;

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_PCM_FUNCS_H_
