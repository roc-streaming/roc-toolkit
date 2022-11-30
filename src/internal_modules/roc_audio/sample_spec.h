/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_audio/sample_spec.h
//! @brief Sample specifications.

#ifndef ROC_AUDIO_SAMPLE_SPEC_H_
#define ROC_AUDIO_SAMPLE_SPEC_H_

#include "roc_audio/sample.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Sample stream specification.
//! Defines sample rate and channel layout.
class SampleSpec {
public:
    //! Default constructor.
    SampleSpec();

    //! Constructor with sample rate and channel mask.
    SampleSpec(size_t sample_rate, packet::channel_mask_t channel_mask);

    //! @name Equality
    //! @{

    //! Check two specifications for equality.
    bool operator==(const SampleSpec& other) const;

    //! Check two specifications for equality.
    bool operator!=(const SampleSpec& other) const;

    // @}

    //! @name Getters and setters
    //! @{

    //! Get sample rate.
    size_t sample_rate() const;

    //! Set sample rate.
    void set_sample_rate(size_t sample_rate);

    //! Get channel mask.
    packet::channel_mask_t channel_mask() const;

    //! Get number of channels.
    size_t num_channels() const;

    //! Set channel mask.
    void set_channel_mask(packet::channel_mask_t channel_mask);

    // @}

    //! @name Nanosecond duration
    //! @{

    //! Convert nanoseconds duration to number of samples per channel.
    size_t ns_2_samples_per_chan(core::nanoseconds_t ns_duration) const;

    //! Convert number of samples per channel to nanoseconds duration.
    core::nanoseconds_t samples_per_chan_2_ns(size_t n_samples) const;

    //! Convert nanoseconds duration to number of samples for all channels.
    size_t ns_2_samples_overall(core::nanoseconds_t ns_duration) const;

    //! Convert number of samples for all channels to nanoseconds duration.
    core::nanoseconds_t samples_overall_2_ns(size_t n_samples) const;

    // @}

    //! @name RTP timestamp
    //! @{

    //! Convert nanoseconds delta to RTP timestamp delta.
    //! @note
    //!  Same as ns_2_samples_per_chan(), but supports negative deltas.
    packet::timestamp_diff_t ns_2_rtp_timestamp(core::nanoseconds_t ns_delta) const;

    //! Convert RTP timestamp delta to nanoseconds delta.
    //! @note
    //!  Same as samples_per_chan_2_ns(), but supports negative deltas.
    core::nanoseconds_t rtp_timestamp_2_ns(packet::timestamp_diff_t rtp_delta) const;

    // @}

private:
    size_t sample_rate_;
    packet::channel_mask_t channel_mask_;
    size_t num_channels_;
};

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_SPEC_H_
