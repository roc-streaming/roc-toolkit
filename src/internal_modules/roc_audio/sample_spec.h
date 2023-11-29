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

#include "roc_audio/channel_set.h"
#include "roc_audio/sample.h"
#include "roc_core/stddefs.h"
#include "roc_core/string_builder.h"
#include "roc_core/time.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

//! Sample specification.
//! Describes sample rate and channels.
class SampleSpec {
public:
    //! Construct empty specification.
    SampleSpec();

    //! Construct specification with parameters.
    SampleSpec(size_t sample_rate, const ChannelSet& channel_set);

    //! Construct specification with parameters.
    //! @remarks
    //!  This is a convenient overload for the case when 32-bit mask is enough to
    //!  describe channels. Otherwise, use overload that accepts ChannelSet.
    SampleSpec(size_t sample_rate,
               ChannelLayout channel_layout,
               ChannelOrder channel_order,
               ChannelMask channel_mask);

    //! @name Equality
    //! @{

    //! Check two specifications for equality.
    bool operator==(const SampleSpec& other) const;

    //! Check two specifications for equality.
    bool operator!=(const SampleSpec& other) const;

    // @}

    //! @name Getters and setters
    //! @{

    //! Check if sample spec has non-zero rate and valid channel set.
    bool is_valid() const;

    //! Unset all fields.
    void clear();

    //! Get sample rate.
    //! @remarks
    //!  Defines sample frequency (number of samples per second).
    size_t sample_rate() const;

    //! Set sample rate.
    void set_sample_rate(size_t sample_rate);

    //! Get channel set.
    //! @remarks
    //!  Defines sample channels (layout and numbers).
    const ChannelSet& channel_set() const;

    //! Get mutable channel set.
    ChannelSet& channel_set();

    //! Set channel set.
    void set_channel_set(const ChannelSet& channel_set);

    //! Get number enabled channels in channel set.
    //! @remarks
    //!  Shorthand for channel_set().num_channels().
    size_t num_channels() const;

    // @}

    //! @name Nanosecond duration converters
    //! @{

    //! Convert nanoseconds duration to number of samples per channel.
    //! @pre
    //!  @p ns_duration should not be negative.
    //! @note
    //!  In case of overflow, result is saturated.
    size_t ns_2_samples_per_chan(core::nanoseconds_t ns_duration) const;

    //! Convert number of samples per channel to nanoseconds duration.
    //! @note
    //!  In case of overflow, result is saturated.
    core::nanoseconds_t samples_per_chan_2_ns(size_t n_samples) const;

    //! Convert (possibly fractional) number samples per channel to nanoseconds duration.
    //! @note
    //!  In case of overflow, result is saturated.
    core::nanoseconds_t fract_samples_per_chan_2_ns(float n_samples) const;

    //! Convert nanoseconds duration to number of samples for all channels.
    //! @pre
    //!  @p ns_duration should not be negative.
    //! @post
    //!  result is always multiple of number of channels.
    //! @note
    //!  In case of overflow, result is saturated.
    size_t ns_2_samples_overall(core::nanoseconds_t ns_duration) const;

    //! Convert number of samples for all channels to nanoseconds duration.
    //! @pre
    //!  @p n_samples should be multiple of number of channels.
    //! @note
    //!  In case of overflow, result is saturated.
    core::nanoseconds_t samples_overall_2_ns(size_t n_samples) const;

    //! Convert number of samples (possibly non-integer) to nanoseconds.
    //! @note
    //!  In case of overflow, result is saturated.
    core::nanoseconds_t fract_samples_overall_2_ns(float n_samples) const;

    // @}

    //! @name RTP timestamp converters
    //! @{

    //! Convert nanoseconds delta to stream timestamp delta.
    //! @remarks
    //!  Same as ns_2_samples_per_chan(), but supports negative deltas.
    packet::stream_timestamp_diff_t
    ns_2_stream_timestamp_delta(core::nanoseconds_t ns_delta) const;

    //! Convert stream timestamp delta to nanoseconds delta.
    //! @remarks
    //!  Same as samples_per_chan_2_ns(), but supports negative deltas.
    core::nanoseconds_t
    stream_timestamp_delta_2_ns(packet::stream_timestamp_diff_t sts_delta) const;

    // @}

private:
    size_t sample_rate_;
    ChannelSet channel_set_;
};

//! Parse sample spec from string.
bool parse_sample_spec(const char* str, SampleSpec& result);

//! Format sample spec to string.
void format_sample_spec(const SampleSpec& sample_spec, core::StringBuilder& bld);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_SPEC_H_
