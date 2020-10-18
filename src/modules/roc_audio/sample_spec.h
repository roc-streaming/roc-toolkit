/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/sample_spec.h

#ifndef ROC_AUDIO_SAMPLE_SPEC_H_
#define ROC_AUDIO_SAMPLE_SPEC_H_

#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Bitmask of channels present in audio packet.
typedef uint32_t channel_mask_t;

//! Default sample rate, number of samples per second.
const size_t DefaultSampleRate = 44100;

//! Default channel mask.
const channel_mask_t DefaultChannelMask = 0x3;

class SampleSpec {
public:
    SampleSpec();
    SampleSpec(size_t sample_rate, channel_mask_t channels);

    ~SampleSpec();

    size_t getSampleRate() const;
    void setSampleRate(size_t sample_rate);
    channel_mask_t getChannels() const;
    void setChannels(channel_mask_t channels);
    size_t num_channels() const;
    packet::timestamp_diff_t timestamp_from_ns(core::nanoseconds_t ns) const;
    core::nanoseconds_t timestamp_to_ns(packet::timestamp_diff_t ts) const;
    size_t ns_to_size(core::nanoseconds_t frame_length) const;
    core::nanoseconds_t size_to_ns(size_t frame_size) const;

private:
    size_t calc_num_channels();

    size_t sample_rate_;
    channel_mask_t channels_;
    size_t num_channels_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_SPEC_H_
