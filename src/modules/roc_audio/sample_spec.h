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

//! Default sample rate, number of samples per second.
const size_t DefaultSampleRate = 44100;

//! Default channel mask.
const packet::channel_mask_t DefaultChannelMask = 0x3;

//! Sample spec.
class SampleSpec {
public:
    //! Default constructor.
    SampleSpec();

    //! Constructor with sample rate and channel mask.
    SampleSpec(size_t sample_rate, packet::channel_mask_t channel_mask);

    //! Get sample rate.
    size_t sample_rate() const;
    
    //! Set sample rate.
    void set_sample_rate(size_t sample_rate);

    //! Get channel mask.
    packet::channel_mask_t channel_mask() const;
    
    //! Set channel mask.
    void set_channel_mask(packet::channel_mask_t channel_mask);

    //! Get number of channels.
    size_t num_channels() const;

    //! Convert nanoseconds to number of samples.
    packet::timestamp_diff_t timestamp_from_ns(core::nanoseconds_t ns) const;

    //! Convert number of samples to nanoseconds.
    core::nanoseconds_t timestamp_to_ns(packet::timestamp_diff_t ts) const;

    //! Convert frame duration to frame size.
    size_t ns_to_size(core::nanoseconds_t frame_length) const;

    //! Convert frame size to frame duration.
    core::nanoseconds_t size_to_ns(size_t frame_size) const;

private:
    size_t calc_num_channels() const;

    size_t sample_rate_;
    packet::channel_mask_t channel_mask_;
    size_t num_channels_;
};


} // namespace audio
} // namespace roc

#endif // ROC_SAMPLE_SPEC_H_
