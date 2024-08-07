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
#include "roc_audio/frame.h"
#include "roc_audio/frame_factory.h"
#include "roc_audio/pcm_format.h"
#include "roc_audio/sample.h"
#include "roc_audio/sample_format.h"
#include "roc_core/attributes.h"
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
    //! @note
    //!  This constructor sets sample_format() to SampleFormat_Pcm.
    SampleSpec(size_t sample_rate, PcmFormat pcm_fmt, const ChannelSet& channel_set);

    //! Construct specification with parameters.
    //! @remarks
    //!  This is a convenient overload for the case when 32-bit mask is enough to
    //!  describe channels. Otherwise, use overload that accepts ChannelSet.
    //! @note
    //!  This constructor sets sample_format() to SampleFormat_Pcm.
    SampleSpec(size_t sample_rate,
               PcmFormat pcm_fmt,
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

    //! Check if sample spec has a zero rate, empty channel set, and invalid_format.
    bool is_empty() const;

    //! Check if samples are in PCM format.
    //! @returns
    //!  true if sample_format() is SampleFormat_Pcm and pcm_format()
    //!  is anything except PcmFormat_Invalid.
    bool is_pcm() const;

    //! Check if samples are in raw format.
    //! @returns
    //!  true if sample_format() is SampleFormat_Pcm and pcm_format()
    //!  is Sample_RawFormat (32-bit native-endian floats).
    bool is_raw() const;

    //! Unset all fields.
    void clear();

    //! Set missing fields from provided defaults.
    //! @remarks
    //!  Updates only those fields which don't have values,
    //!  with corresponding values provided as arguments.
    void use_defaults(PcmFormat default_pcm_fmt,
                      ChannelLayout default_channel_layout,
                      ChannelOrder default_channel_order,
                      ChannelMask default_channel_mask,
                      size_t default_sample_rate);

    //! Get sample rate.
    //! @remarks
    //!  Defines sample frequency (number of samples per second).
    size_t sample_rate() const;

    //! Set sample rate.
    void set_sample_rate(size_t sample_rate);

    //! Get sample format.
    //! @remarks
    //!  Defines how samples are represented in memory.
    //!  When set to SampleFormat_Pcm, pcm_format() defines what exact PCM coding
    //!  and endian are used.
    SampleFormat sample_format() const;

    //! Set sample format.
    void set_sample_format(SampleFormat sample_fmt);

    //! Get PCM format.
    //! @remarks
    //!  When sample_format() is set to SampleFormat_Pcm, defines what exact PCM coding
    //!  and endian are used.
    PcmFormat pcm_format() const;

    //! Set PCM format.
    void set_pcm_format(PcmFormat pcm_fmt);

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

    //! @name Convert number of samples
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

    //! @name Convert stream timestamps
    //! @{

    //! Convert nanoseconds delta to stream timestamp.
    //! @pre
    //!  @p ns_duration should not be negative.
    //! @remarks
    //!  Same as ns_2_samples_per_chan(), but with stream_timestamp_t instead of size_t.
    packet::stream_timestamp_t
    ns_2_stream_timestamp(core::nanoseconds_t ns_duration) const;

    //! Convert stream timestamp to nanoseconds.
    //! @remarks
    //!  Same as samples_per_chan_2_ns(), but with stream_timestamp_t instead of size_t.
    core::nanoseconds_t
    stream_timestamp_2_ns(packet::stream_timestamp_t sts_duration) const;

    //! Convert stream timestamp to milliseconds.
    double stream_timestamp_2_ms(packet::stream_timestamp_t sts_duration) const;

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

    //! Convert stream timestamp delta to milliseconds.
    double
    stream_timestamp_delta_2_ms(packet::stream_timestamp_diff_t sts_duration) const;

    // @}

    //! @name Convert byte size
    //! @{

    //! Convert byte size to stream timestamp.
    //! @pre
    //!  sample_format() should be PCM.
    packet::stream_timestamp_t bytes_2_stream_timestamp(size_t n_bytes) const;

    //! Convert stream timestamp to byte size.
    //! @pre
    //!  sample_format() should be PCM.
    size_t stream_timestamp_2_bytes(packet::stream_timestamp_t duration) const;

    //! Convert byte size to nanosecond duration.
    //! @pre
    //!  sample_format() should be PCM.
    core::nanoseconds_t bytes_2_ns(size_t n_bytes) const;

    //! Convert nanosecond duration to byte size.
    //! @pre
    //!  sample_format() should be PCM.
    size_t ns_2_bytes(core::nanoseconds_t duration) const;

    // @}

    //! @name Frame helpers
    //! @{

    //! Check if frame corresponds to the sample spec.
    //! Panic if something is wrong.
    void validate_frame(Frame& frame) const;

    //! Check if frame size is multiple of sample size and channel count.
    //! Returns false if size is invalid.
    bool is_valid_frame_size(size_t n_bytes);

    //! Cap duration to fit given buffer size in bytes.
    //! Returns @p duration or smaller value.
    packet::stream_timestamp_t cap_frame_duration(packet::stream_timestamp_t duration,
                                                  size_t n_bytes) const;

    // @}

private:
    size_t sample_rate_;
    SampleFormat sample_fmt_;
    PcmFormat pcm_fmt_;
    size_t pcm_width_;
    ChannelSet channel_set_;
};

//! Parse sample spec from string.
//!
//! @remarks
//!  The input string should have the form:
//!   - "<format>/<rate>/<channels>"
//!
//!  Where:
//!   - "<format>" is string name of sample format (e.g. "s16")
//!   - "<rate>" is a positive integer
//!   - "<channels>" can be: "<surround preset>", "<surround channel list>",
//!                        "<multitrack mask>", "<multitrack channel list>"
//!
//!   - "<surround preset>" is a string name of predefined surround channel
//!                         mask, e.g. "stereo", "surround4.1", etc.
//!   - "<surround channel list>" is comma-separated list of surround channel names,
//!                               e.g. "FL,FC,FR"
//!
//!   - "<multitrack mask>" is a 1024-bit hex mask defining which tracks are
//!                         enabled, e.g. "0xAA00BB00"
//!   - "<multitrack channel list>" is a comma-separated list of track numbers
//!                                 or ranges, e.g. "1,2,5-8"
//!
//! Each of the three components ("<format>", "<rate>", "<channels>") may be set
//! to "-", which means "keep unset".
//!
//! All four forms of "<channels>" component are alternative ways to represent a
//! bitmask of enabled channels or tracks. The order of channels does no matter.
//!
//! Examples:
//!  - "s16/44100/stereo"
//!  - "s18_4le/48000/FL,FC,FR"
//!  - "f32/96000/1,2,10-20,31"
//!  - "f32/96000/0xA0000000FFFF0000000C"
//!  - "-/44100/-"
//!  - "-/-/-"
//!
//! @returns
//!  false if string can't be parsed.
ROC_ATTR_NODISCARD bool parse_sample_spec(const char* str, SampleSpec& result);

//! Format sample spec to string.
void format_sample_spec(const SampleSpec& sample_spec, core::StringBuilder& bld);

} // namespace audio
} // namespace roc

#endif // ROC_AUDIO_SAMPLE_SPEC_H_
