/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_spec.h"
#include "roc_audio/pcm_format.h"
#include "roc_audio/sample_format.h"
#include "roc_audio/sample_spec_to_str.h"
#include "roc_core/macro_helpers.h"
#include "roc_core/panic.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

namespace {

float ns_2_fract_samples(const core::nanoseconds_t ns, const size_t sample_rate) {
    return roundf(float(ns) / core::Second * sample_rate);
}

template <class T>
T ns_2_int_samples(const core::nanoseconds_t ns,
                   const size_t sample_rate,
                   const size_t multiplier) {
    const T min_val = ROC_MIN_OF(T);
    const T max_val = ROC_MAX_OF(T);

    const T mul = (T)multiplier;

    const float val = ns_2_fract_samples(ns, sample_rate);

    if (val * multiplier <= (float)min_val) {
        return min_val / mul * mul;
    }

    if (val * multiplier >= (float)max_val) {
        return max_val / mul * mul;
    }

    return (T)val * mul;
}

core::nanoseconds_t nsamples_2_ns(const float n_samples, const size_t sample_rate) {
    const core::nanoseconds_t min_val = ROC_MIN_OF(core::nanoseconds_t);
    const core::nanoseconds_t max_val = ROC_MAX_OF(core::nanoseconds_t);

    const float val = roundf(n_samples / sample_rate * core::Second);

    if (val <= (float)min_val) {
        return min_val;
    }

    if (val >= (float)max_val) {
        return max_val;
    }

    return (core::nanoseconds_t)val;
}

PcmFormat get_pcm_canon_format(PcmFormat fmt) {
    if (fmt == PcmFormat_Invalid) {
        return PcmFormat_Invalid;
    }

    const PcmTraits traits = pcm_format_traits(fmt);
    return traits.canon_id;
}

size_t get_pcm_sample_width(PcmFormat fmt) {
    if (fmt == PcmFormat_Invalid) {
        return 0;
    }

    const PcmTraits traits = pcm_format_traits(fmt);
    return traits.bit_width;
}

} // namespace

SampleSpec::SampleSpec()
    : sample_rate_(0)
    , sample_fmt_(SampleFormat_Invalid)
    , pcm_fmt_(PcmFormat_Invalid)
    , pcm_width_(0) {
}

SampleSpec::SampleSpec(const size_t sample_rate,
                       const PcmFormat pcm_fmt,
                       const ChannelSet& channel_set)
    : sample_rate_(sample_rate)
    , sample_fmt_(SampleFormat_Pcm)
    , pcm_fmt_(pcm_fmt)
    , pcm_width_(get_pcm_sample_width(pcm_fmt))
    , channel_set_(channel_set) {
    roc_panic_if_msg(sample_rate_ == 0, "sample spec: invalid sample rate");
    roc_panic_if_msg(pcm_fmt_ == PcmFormat_Invalid || pcm_width_ == 0,
                     "sample spec: invalid pcm format");
    roc_panic_if_msg(!channel_set_.is_valid(), "sample spec: invalid channel set");
}

SampleSpec::SampleSpec(const size_t sample_rate,
                       const PcmFormat pcm_fmt,
                       const ChannelLayout channel_layout,
                       ChannelOrder channel_order,
                       const ChannelMask channel_mask)
    : sample_rate_(sample_rate)
    , sample_fmt_(SampleFormat_Pcm)
    , pcm_fmt_(pcm_fmt)
    , pcm_width_(get_pcm_sample_width(pcm_fmt))
    , channel_set_(channel_layout, channel_order, channel_mask) {
    roc_panic_if_msg(sample_rate_ == 0, "sample spec: invalid sample rate");
    roc_panic_if_msg(pcm_fmt_ == PcmFormat_Invalid || pcm_width_ == 0,
                     "sample spec: invalid pcm format");
    roc_panic_if_msg(!channel_set_.is_valid(), "sample spec: invalid channel set");
}

bool SampleSpec::operator==(const SampleSpec& other) const {
    return sample_fmt_ == other.sample_fmt_
        && (sample_fmt_ != SampleFormat_Pcm || pcm_fmt_ == other.pcm_fmt_
            || get_pcm_canon_format(pcm_fmt_) == get_pcm_canon_format(other.pcm_fmt_))
        && sample_rate_ == other.sample_rate_ && channel_set_ == other.channel_set_;
}

bool SampleSpec::operator!=(const SampleSpec& other) const {
    return !(*this == other);
}

bool SampleSpec::is_valid() const {
    return sample_fmt_ != SampleFormat_Invalid
        && ((sample_fmt_ == SampleFormat_Pcm) == (pcm_fmt_ != PcmFormat_Invalid))
        && sample_rate_ != 0 && channel_set_.is_valid();
}

bool SampleSpec::is_raw() const {
    return sample_fmt_ == SampleFormat_Pcm && pcm_fmt_ == Sample_RawFormat;
}

void SampleSpec::clear() {
    sample_fmt_ = SampleFormat_Invalid;
    pcm_fmt_ = PcmFormat_Invalid;
    pcm_width_ = 0;
    sample_rate_ = 0;
    channel_set_.clear();
}

SampleFormat SampleSpec::sample_format() const {
    return sample_fmt_;
}

void SampleSpec::set_sample_format(SampleFormat sample_fmt) {
    sample_fmt_ = sample_fmt;
}

PcmFormat SampleSpec::pcm_format() const {
    return pcm_fmt_;
}

void SampleSpec::set_pcm_format(PcmFormat pcm_fmt) {
    pcm_fmt_ = pcm_fmt;
    pcm_width_ = get_pcm_sample_width(pcm_fmt);
}

size_t SampleSpec::sample_rate() const {
    return sample_rate_;
}

void SampleSpec::set_sample_rate(const size_t sample_rate) {
    sample_rate_ = sample_rate;
}

const ChannelSet& SampleSpec::channel_set() const {
    return channel_set_;
}

ChannelSet& SampleSpec::channel_set() {
    return channel_set_;
}

void SampleSpec::set_channel_set(const ChannelSet& channel_set) {
    channel_set_ = channel_set;
}

size_t SampleSpec::num_channels() const {
    return channel_set_.num_channels();
}

size_t SampleSpec::ns_2_samples_per_chan(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return ns_2_int_samples<size_t>(ns_duration, sample_rate_, 1);
}

core::nanoseconds_t SampleSpec::samples_per_chan_2_ns(const size_t n_samples) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns((float)n_samples, sample_rate_);
}

core::nanoseconds_t SampleSpec::fract_samples_per_chan_2_ns(const float n_samples) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns(n_samples, sample_rate_);
}

size_t SampleSpec::ns_2_samples_overall(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return ns_2_int_samples<size_t>(ns_duration, sample_rate_, num_channels());
}

core::nanoseconds_t SampleSpec::samples_overall_2_ns(const size_t n_samples) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(n_samples % num_channels() != 0,
                     "sample spec: # of samples must be dividable by channels number");

    return nsamples_2_ns((float)n_samples / num_channels(), sample_rate_);
}

core::nanoseconds_t SampleSpec::fract_samples_overall_2_ns(const float n_samples) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns(n_samples / num_channels(), sample_rate_);
}

packet::stream_timestamp_t
SampleSpec::ns_2_stream_timestamp(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return ns_2_int_samples<packet::stream_timestamp_t>(ns_duration, sample_rate_, 1);
}

core::nanoseconds_t
SampleSpec::stream_timestamp_2_ns(const packet::stream_timestamp_t sts_duration) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns((float)sts_duration, sample_rate_);
}

double SampleSpec::stream_timestamp_2_ms(packet::stream_timestamp_t sts_duration) const {
    return (double)stream_timestamp_2_ns(sts_duration) / core::Millisecond;
}

packet::stream_timestamp_diff_t
SampleSpec::ns_2_stream_timestamp_delta(const core::nanoseconds_t ns_delta) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return ns_2_int_samples<packet::stream_timestamp_diff_t>(ns_delta, sample_rate_, 1);
}

core::nanoseconds_t SampleSpec::stream_timestamp_delta_2_ns(
    const packet::stream_timestamp_diff_t sts_delta) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns((float)sts_delta, sample_rate_);
}

double
SampleSpec::stream_timestamp_delta_2_ms(packet::stream_timestamp_diff_t sts_delta) const {
    return (double)stream_timestamp_delta_2_ns(sts_delta) / core::Millisecond;
}

packet::stream_timestamp_t SampleSpec::bytes_2_stream_timestamp(size_t n_bytes) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(sample_fmt_ != SampleFormat_Pcm,
                     "sample spec: sample format is not pcm: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(pcm_width_ % 8 != 0,
                     "sample spec: sample width is not byte-aligned: %s",
                     sample_spec_to_str(*this).c_str());

    return n_bytes / (pcm_width_ / 8) / channel_set_.num_channels();
}

size_t SampleSpec::stream_timestamp_2_bytes(packet::stream_timestamp_t duration) const {
    roc_panic_if_msg(!is_valid(), "sample spec: attempt to use invalid spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(sample_fmt_ != SampleFormat_Pcm,
                     "sample spec: sample format is not pcm: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(pcm_width_ % 8 != 0,
                     "sample spec: sample width is not byte-aligned: %s",
                     sample_spec_to_str(*this).c_str());

    return duration * (pcm_width_ / 8) * channel_set_.num_channels();
}

core::nanoseconds_t SampleSpec::bytes_2_ns(size_t n_bytes) const {
    return stream_timestamp_2_ns(bytes_2_stream_timestamp(n_bytes));
}

size_t SampleSpec::ns_2_bytes(core::nanoseconds_t duration) const {
    return stream_timestamp_2_bytes(ns_2_stream_timestamp(duration));
}

} // namespace audio
} // namespace roc
