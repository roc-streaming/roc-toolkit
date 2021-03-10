/*
 * Copyright (c) 2021 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/sample_spec.h
//! @brief Sample specifications

#ifndef ROC_SAMPLE_SPEC_H_
#define ROC_SAMPLE_SPEC_H_

#include "roc_audio/units.h"
#include "roc_packet/units.h"
#include "roc_core/time.h"

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
    SampleSpec(size_t sample_rate, channel_mask_t channel_mask);

    size_t sample_rate() const;
    
    void set_sample_rate(size_t sample_rate);

    channel_mask_t channel_mask() const;
    
    void set_channel_mask(channel_mask_t channel_mask);

    size_t num_channels() const;

    packet::timestamp_diff_t timestamp_from_ns(core::nanoseconds_t ns) const;
    core::nanoseconds_t timestamp_to_ns(packet::timestamp_diff_t ts) const;

private:
    size_t calc_num_channels() const;

    size_t sample_rate_;
    channel_mask_t channel_mask_;
    size_t num_channels_;
};


} // namespace audio
} // namespace roc

#endif // ROC_SAMPLE_SPEC_H_
