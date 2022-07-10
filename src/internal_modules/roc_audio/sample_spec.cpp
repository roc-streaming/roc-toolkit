/*
 * Copyright (c) 2021 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_spec.h"
#include "roc_core/panic.h"
#include "roc_packet/units.h"

namespace roc {
namespace audio {

SampleSpec::SampleSpec() {
    set_sample_rate(0);
    set_channel_mask(0);
}

SampleSpec::SampleSpec(const size_t sample_rate,
                       const packet::channel_mask_t channel_mask) {
    roc_panic_if(sample_rate == 0);
    roc_panic_if(channel_mask == 0);
    set_sample_rate(sample_rate);
    set_channel_mask(channel_mask);
}

size_t SampleSpec::sample_rate() const {
    return sample_rate_;
}

void SampleSpec::set_sample_rate(const size_t sample_rate) {
    sample_rate_ = sample_rate;
}

packet::channel_mask_t SampleSpec::channel_mask() const {
    return channel_mask_;
}

size_t SampleSpec::num_channels() const {
    return num_channels_;
}

void SampleSpec::set_channel_mask(const packet::channel_mask_t channel_mask) {
    channel_mask_ = channel_mask;
    num_channels_ = packet::num_channels(channel_mask);
}

size_t SampleSpec::ns_2_samples_per_chan(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return (size_t)ns_2_rtp_timestamp(ns_duration);
}

core::nanoseconds_t SampleSpec::samples_per_chan_2_ns(const size_t n_samples) const {
    return rtp_timestamp_2_ns((packet::timestamp_diff_t)n_samples);
}

size_t SampleSpec::ns_2_samples_overall(const core::nanoseconds_t ns_duration) const {
    roc_panic_if_msg(ns_duration < 0, "sample spec: duration should not be negative");

    return (size_t)ns_2_rtp_timestamp(ns_duration) * num_channels();
}

core::nanoseconds_t SampleSpec::samples_overall_2_ns(const size_t n_samples) const {
    roc_panic_if_msg(n_samples % num_channels() != 0,
                     "sample spec: # of samples must be dividable by channels number");

    return rtp_timestamp_2_ns(packet::timestamp_diff_t(n_samples / num_channels()));
}

packet::timestamp_diff_t
SampleSpec::ns_2_rtp_timestamp(const core::nanoseconds_t ns_delta) const {
    return packet::timestamp_diff_t(
        roundf(float(ns_delta) / core::Second * sample_rate_));
}

core::nanoseconds_t
SampleSpec::rtp_timestamp_2_ns(const packet::timestamp_diff_t rtp_delta) const {
    return core::nanoseconds_t(roundf(float(rtp_delta) / sample_rate_ * core::Second));
}

} // namespace audio
} // namespace roc
