/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_spec.h"
#include "roc_audio/format.h"
#include "roc_audio/pcm_subformat.h"
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

PcmSubformat get_pcm_portable_format(PcmSubformat fmt) {
    if (fmt == PcmSubformat_Invalid) {
        return PcmSubformat_Invalid;
    }

    const PcmTraits traits = pcm_subformat_traits(fmt);
    return traits.portable_alias;
}

size_t get_pcm_sample_width(PcmSubformat fmt) {
    if (fmt == PcmSubformat_Invalid) {
        return 0;
    }

    const PcmTraits traits = pcm_subformat_traits(fmt);
    return traits.bit_width;
}

} // namespace

SampleSpec::SampleSpec()
    : fmt_(Format_Invalid)
    , has_subfmt_(false)
    , pcm_subfmt_(PcmSubformat_Invalid)
    , pcm_subfmt_width_(0)
    , sample_rate_(0)
    , channel_set_() {
    fmt_name_[0] = '\0';
    subfmt_name_[0] = '\0';
}

SampleSpec::SampleSpec(const size_t sample_rate,
                       const PcmSubformat pcm_fmt,
                       const ChannelSet& channel_set)
    : fmt_(Format_Invalid)
    , has_subfmt_(false)
    , pcm_subfmt_(PcmSubformat_Invalid)
    , pcm_subfmt_width_(0)
    , sample_rate_(0)
    , channel_set_(channel_set) {
    fmt_name_[0] = '\0';
    subfmt_name_[0] = '\0';

    set_format(Format_Pcm);
    set_pcm_subformat(pcm_fmt);
    set_sample_rate(sample_rate);

    roc_panic_if_msg(!is_complete(),
                     "sample spec: attempt to construct incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());
}

SampleSpec::SampleSpec(const size_t sample_rate,
                       const PcmSubformat pcm_fmt,
                       const ChannelLayout channel_layout,
                       ChannelOrder channel_order,
                       const ChannelMask channel_mask)
    : fmt_(Format_Invalid)
    , has_subfmt_(false)
    , pcm_subfmt_(PcmSubformat_Invalid)
    , pcm_subfmt_width_(0)
    , sample_rate_(0)
    , channel_set_(channel_layout, channel_order, channel_mask) {
    fmt_name_[0] = '\0';
    subfmt_name_[0] = '\0';

    set_format(Format_Pcm);
    set_pcm_subformat(pcm_fmt);
    set_sample_rate(sample_rate);

    roc_panic_if_msg(!is_complete(),
                     "sample spec: attempt to construct incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());
}

bool SampleSpec::operator==(const SampleSpec& other) const {
    // format
    if (has_format() || other.has_format()) {
        if (fmt_ != other.fmt_) {
            return false;
        }
        if (fmt_ == Format_Custom && strcmp(fmt_name_, other.fmt_name_) != 0) {
            return false;
        }
    }

    // sub-format
    if (has_subformat() || other.has_subformat()) {
        if (pcm_subfmt_ != PcmSubformat_Invalid
            || other.pcm_subfmt_ != PcmSubformat_Invalid) {
            if (get_pcm_portable_format(pcm_subfmt_)
                != get_pcm_portable_format(other.pcm_subfmt_)) {
                return false;
            }
        } else {
            if (strcmp(subfmt_name_, other.subfmt_name_) != 0) {
                return false;
            }
        }
    }

    // rate, channels
    if (sample_rate_ != other.sample_rate_) {
        return false;
    }
    if (channel_set_ != other.channel_set_) {
        return false;
    }

    return true;
}

bool SampleSpec::operator!=(const SampleSpec& other) const {
    return !(*this == other);
}

bool SampleSpec::is_complete() const {
    // format
    if (!has_format()) {
        return false;
    }
    if (!fmt_name_[0]) {
        return false;
    }

    // sub-format
    if (fmt_ == Format_Pcm) {
        if (pcm_subfmt_ == PcmSubformat_Invalid) {
            return false;
        }
    }
    if (has_subformat()) {
        if (!subfmt_name_[0]) {
            return false;
        }
    }

    // rate, channels
    if (!has_sample_rate()) {
        return false;
    }
    if (!has_channel_set()) {
        return false;
    }

    return true;
}

bool SampleSpec::is_empty() const {
    return fmt_ == Format_Invalid && !has_subfmt_ && pcm_subfmt_ == PcmSubformat_Invalid
        && sample_rate_ == 0 && channel_set_.num_channels() == 0;
}

bool SampleSpec::is_pcm() const {
    return fmt_ == Format_Pcm && pcm_subfmt_ != PcmSubformat_Invalid;
}

bool SampleSpec::is_raw() const {
    return fmt_ == Format_Pcm
        && get_pcm_portable_format(pcm_subfmt_)
        == get_pcm_portable_format(PcmSubformat_Raw);
}

void SampleSpec::clear() {
    fmt_ = Format_Invalid;
    fmt_name_[0] = '\0';

    has_subfmt_ = false;
    subfmt_name_[0] = '\0';
    pcm_subfmt_ = PcmSubformat_Invalid;
    pcm_subfmt_width_ = 0;

    sample_rate_ = 0;
    channel_set_.clear();
}

void SampleSpec::use_defaults(Format default_fmt,
                              PcmSubformat default_pcm_fmt,
                              ChannelLayout default_channel_layout,
                              ChannelOrder default_channel_order,
                              ChannelMask default_channel_mask,
                              size_t default_sample_rate) {
    if (!has_format() && default_fmt != Format_Invalid) {
        set_format(default_fmt);
    }

    if (!has_subformat() && default_pcm_fmt != PcmSubformat_Invalid) {
        set_pcm_subformat(default_pcm_fmt);
    }

    if (!has_sample_rate() && default_sample_rate != 0) {
        set_sample_rate(default_sample_rate);
    }

    if (!has_channel_set() && default_channel_layout != ChanLayout_None) {
        channel_set_.set_layout(default_channel_layout);
        channel_set_.set_order(default_channel_order);
        channel_set_.set_mask(default_channel_mask);
    }
}

bool SampleSpec::has_format() const {
    return fmt_ != Format_Invalid;
}

Format SampleSpec::format() const {
    return fmt_;
}

const char* SampleSpec::format_name() const {
    return fmt_name_;
}

void SampleSpec::set_format(Format fmt) {
    roc_panic_if_msg(fmt < Format_Invalid || fmt >= Format_Max,
                     "sample spec: invalid format id");

    if (fmt_ == fmt) {
        return;
    }

    fmt_ = fmt;

    if (fmt_ == Format_Invalid || fmt_ == Format_Custom) {
        strcpy(fmt_name_, "");
    } else {
        strcpy(fmt_name_, format_to_str(fmt));
    }
}

bool SampleSpec::set_custom_format(const char* name) {
    roc_panic_if_msg(!name, "sample spec: invalid null string");

    const size_t name_len = strlen(name);
    if (name_len == 0 || name_len >= MaxNameLen) {
        return false;
    }

    fmt_ = Format_Custom;
    strcpy(fmt_name_, name);

    return true;
}

bool SampleSpec::has_subformat() const {
    return has_subfmt_;
}

const char* SampleSpec::subformat_name() const {
    return subfmt_name_;
}

PcmSubformat SampleSpec::pcm_subformat() const {
    return pcm_subfmt_;
}

size_t SampleSpec::pcm_bit_width() const {
    return pcm_subfmt_width_;
}

void SampleSpec::set_pcm_subformat(PcmSubformat pcm_fmt) {
    roc_panic_if_msg(pcm_fmt < PcmSubformat_Invalid || pcm_fmt >= PcmSubformat_Max,
                     "sample spec: invalid pcm format id");

    if (pcm_subfmt_ == pcm_fmt) {
        return;
    }

    pcm_subfmt_ = pcm_fmt;
    pcm_subfmt_width_ = get_pcm_sample_width(pcm_fmt);

    if (pcm_subfmt_ == PcmSubformat_Invalid) {
        has_subfmt_ = false;
        strcpy(subfmt_name_, "");
    } else {
        has_subfmt_ = true;
        strcpy(subfmt_name_, pcm_subformat_to_str(pcm_subfmt_));
    }
}

bool SampleSpec::set_custom_subformat(const char* name) {
    roc_panic_if_msg(!name, "sample spec: string is null");

    const size_t name_len = strlen(name);
    if (name_len == 0 || name_len >= MaxNameLen) {
        return false;
    }

    pcm_subfmt_ = PcmSubformat_Invalid;
    pcm_subfmt_width_ = 0;

    has_subfmt_ = true;
    strcpy(subfmt_name_, name);

    return true;
}

bool SampleSpec::has_sample_rate() const {
    return sample_rate_ != 0;
}

size_t SampleSpec::sample_rate() const {
    return sample_rate_;
}

void SampleSpec::set_sample_rate(const size_t sample_rate) {
    sample_rate_ = sample_rate;
}

bool SampleSpec::has_channel_set() const {
    return channel_set_.is_valid();
}

size_t SampleSpec::num_channels() const {
    return channel_set_.num_channels();
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

size_t SampleSpec::ns_2_samples_per_chan(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return ns_2_int_samples<size_t>(ns_duration, sample_rate_, 1);
}

core::nanoseconds_t SampleSpec::samples_per_chan_2_ns(const size_t n_samples) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns((float)n_samples, sample_rate_);
}

core::nanoseconds_t SampleSpec::fract_samples_per_chan_2_ns(const float n_samples) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns(n_samples, sample_rate_);
}

size_t SampleSpec::ns_2_samples_overall(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return ns_2_int_samples<size_t>(ns_duration, sample_rate_, num_channels());
}

core::nanoseconds_t SampleSpec::samples_overall_2_ns(const size_t n_samples) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(n_samples % num_channels() != 0,
                     "sample spec: # of samples must be dividable by channels number");

    return nsamples_2_ns((float)n_samples / num_channels(), sample_rate_);
}

core::nanoseconds_t SampleSpec::fract_samples_overall_2_ns(const float n_samples) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns(n_samples / num_channels(), sample_rate_);
}

packet::stream_timestamp_t
SampleSpec::ns_2_stream_timestamp(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return ns_2_int_samples<packet::stream_timestamp_t>(ns_duration, sample_rate_, 1);
}

core::nanoseconds_t
SampleSpec::stream_timestamp_2_ns(const packet::stream_timestamp_t sts_duration) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns((float)sts_duration, sample_rate_);
}

double SampleSpec::stream_timestamp_2_ms(packet::stream_timestamp_t sts_duration) const {
    return (double)stream_timestamp_2_ns(sts_duration) / core::Millisecond;
}

packet::stream_timestamp_diff_t
SampleSpec::ns_2_stream_timestamp_delta(const core::nanoseconds_t ns_delta) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    return ns_2_int_samples<packet::stream_timestamp_diff_t>(ns_delta, sample_rate_, 1);
}

core::nanoseconds_t SampleSpec::stream_timestamp_delta_2_ns(
    const packet::stream_timestamp_diff_t sts_delta) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    return nsamples_2_ns((float)sts_delta, sample_rate_);
}

double
SampleSpec::stream_timestamp_delta_2_ms(packet::stream_timestamp_diff_t sts_delta) const {
    return (double)stream_timestamp_delta_2_ns(sts_delta) / core::Millisecond;
}

packet::stream_timestamp_t SampleSpec::bytes_2_stream_timestamp(size_t n_bytes) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(fmt_ != Format_Pcm, "sample spec: sample format is not pcm: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(pcm_subfmt_width_ % 8 != 0,
                     "sample spec: sample width is not byte-aligned: %s",
                     sample_spec_to_str(*this).c_str());

    return n_bytes / (pcm_subfmt_width_ / 8) / channel_set_.num_channels();
}

size_t SampleSpec::stream_timestamp_2_bytes(packet::stream_timestamp_t duration) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(fmt_ != Format_Pcm, "sample spec: sample format is not pcm: %s",
                     sample_spec_to_str(*this).c_str());

    roc_panic_if_msg(pcm_subfmt_width_ % 8 != 0,
                     "sample spec: sample width is not byte-aligned: %s",
                     sample_spec_to_str(*this).c_str());

    return duration * (pcm_subfmt_width_ / 8) * channel_set_.num_channels();
}

core::nanoseconds_t SampleSpec::bytes_2_ns(size_t n_bytes) const {
    return stream_timestamp_2_ns(bytes_2_stream_timestamp(n_bytes));
}

size_t SampleSpec::ns_2_bytes(core::nanoseconds_t duration) const {
    return stream_timestamp_2_bytes(ns_2_stream_timestamp(duration));
}

void SampleSpec::validate_frame(Frame& frame) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    if (frame.num_bytes() == 0) {
        roc_panic("sample spec: invalid frame: no bytes: spec=%s",
                  sample_spec_to_str(*this).c_str());
    }

    if (!frame.has_duration()) {
        roc_panic("sample spec: invalid frame: no duration: spec=%s",
                  sample_spec_to_str(*this).c_str());
    }

    if (frame.capture_timestamp() < 0) {
        roc_panic("sample spec: invalid frame: negative cts: spec=%s",
                  sample_spec_to_str(*this).c_str());
    }

    if (is_raw()) {
        if (!frame.is_raw()) {
            roc_panic("sample spec: invalid frame: expected raw format: spec=%s",
                      sample_spec_to_str(*this).c_str());
        }

        if (frame.duration() * num_channels() != frame.num_raw_samples()
            || frame.num_raw_samples() * sizeof(sample_t) != frame.num_bytes()) {
            roc_panic("sample spec: invalid frame: mismatching sizes:"
                      " n_samples=%lu n_bytes=%lu duration=%lu spec=%s",
                      (unsigned long)frame.num_raw_samples(),
                      (unsigned long)frame.num_bytes(), (unsigned long)frame.duration(),
                      sample_spec_to_str(*this).c_str());
        }
    } else {
        if (frame.is_raw()) {
            roc_panic("sample spec: invalid frame: expected non-raw format: spec=%s",
                      sample_spec_to_str(*this).c_str());
        }
    }
}

bool SampleSpec::is_valid_frame_size(size_t n_bytes) {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    if (fmt_ != Format_Pcm || pcm_subfmt_width_ % 8 != 0) {
        return true;
    }

    const size_t factor = stream_timestamp_2_bytes(1);

    if (n_bytes % factor == 0) {
        return true;
    }

    roc_log(LogError,
            "sample spec: invalid frame buffer size: should be multiple of %u, got %lu"
            " (%u bytes per sample, %u channels)",
            (unsigned)factor, (unsigned long)n_bytes, (unsigned)(pcm_subfmt_width_ / 8),
            (unsigned)num_channels());

    return false;
}

packet::stream_timestamp_t
SampleSpec::cap_frame_duration(packet::stream_timestamp_t duration,
                               size_t buffer_size) const {
    roc_panic_if_msg(!is_complete(), "sample spec: attempt to use incomplete spec: %s",
                     sample_spec_to_str(*this).c_str());

    return std::min(duration, bytes_2_stream_timestamp(buffer_size));
}

} // namespace audio
} // namespace roc
