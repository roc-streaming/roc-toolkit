/*
 * Copyright (c) 2021 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_spec.h"

namespace roc {
namespace audio {

SampleSpec::SampleSpec() {
    sample_rate_ = DefaultSampleRate;
    set_channel_mask(DefaultChannelMask);
}

SampleSpec::SampleSpec(size_t sample_rate, 
                       channel_mask_t channel_mask)
    : sample_rate_(sample_rate) {
    set_channel_mask(channel_mask);
}

size_t SampleSpec::get_sample_rate() const {
    return sample_rate_;
}

void SampleSpec::set_sample_rate(size_t sample_rate) {
    sample_rate_ = sample_rate;
}

channel_mask_t SampleSpec::get_channel_mask() const {
    return channel_mask_;
}

void SampleSpec::set_channel_mask(channel_mask_t channel_mask) {
    channel_mask_ = channel_mask;
    num_channels_ = calc_num_channels();
}

size_t SampleSpec::num_channels() const {
    return num_channels_;
}

size_t SampleSpec::calc_num_channels() const {
    size_t n_ch = 0;
    channel_mask_t ch_mask = channel_mask_;
    for (; ch_mask != 0; ch_mask >>= 1) {
        if (ch_mask & 1) {
            n_ch++;
        }
    }
    return n_ch;
}

} // namespace audio
} // namespace roc