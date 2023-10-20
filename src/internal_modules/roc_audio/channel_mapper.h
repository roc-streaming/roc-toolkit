/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/channel_mapper.h
//! @brief Mixer.

#ifndef ROC_AUDIO_CHANNEL_MAPPER_H_
#define ROC_AUDIO_CHANNEL_MAPPER_H_

#include "roc_audio/channel_mapper_matrix.h"
#include "roc_audio/channel_set.h"
#include "roc_audio/frame.h"
#include "roc_core/buffer_factory.h"
#include "roc_core/noncopyable.h"

namespace roc {
namespace audio {

//! Channel mapper.
//! Converts between frames with specified channel masks.
class ChannelMapper : public core::NonCopyable<> {
public:
    //! Initialize.
    ChannelMapper(const ChannelSet& in_chans, const ChannelSet& out_chans);

    //! Map samples.
    void map(const sample_t* in_samples,
             size_t n_in_samples,
             sample_t* out_samples,
             size_t n_out_samples);

private:
    typedef void (ChannelMapper::*map_func_t)(const sample_t* in_samples,
                                              sample_t* out_samples,
                                              size_t n_samples);

    void map_surround_surround_(const sample_t* in_samples,
                                sample_t* out_samples,
                                size_t n_samples);
    void map_multitrack_surround_(const sample_t* in_samples,
                                  sample_t* out_samples,
                                  size_t n_samples);
    void map_multitrack_multitrack_(const sample_t* in_samples,
                                    sample_t* out_samples,
                                    size_t n_samples);

    void setup_map_func_();

    const ChannelSet in_chans_;
    const ChannelSet out_chans_;
    ChannelSet inout_chans_;

    ChannelMapperMatrix matrix_;
    map_func_t map_func_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_CHANNEL_MAPPER_H_
