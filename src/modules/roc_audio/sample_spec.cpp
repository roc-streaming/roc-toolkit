/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_audio/sample_spec.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace audio {

SampleSpec::SampleSpec() {
    sample_rate_ = DefaultSampleRate;
    channels_ = DefaultChannelMask;
    num_channels_ = calc_num_channels();
}

SampleSpec::SampleSpec(size_t sample_rate,
                       channel_mask_t channels) {
    sample_rate_ = sample_rate;
    channels_ = channels;
    num_channels_ = calc_num_channels();
}

size_t SampleSpec::getSampleRate() const {
    return sample_rate_;
}

void SampleSpec::setSampleRate(size_t sample_rate) {
    sample_rate_ = sample_rate;
}

channel_mask_t SampleSpec::getChannels() const {
    return channels_;
}

void SampleSpec::setChannels(channel_mask_t channels) {
    channels_ = channels;
    num_channels_ = calc_num_channels();
}

size_t SampleSpec::num_channels() const {
    return num_channels_;
}

packet::timestamp_diff_t SampleSpec::timestamp_from_ns(core::nanoseconds_t ns) const {
    return packet::timestamp_diff_t(roundf(float(ns) / core::Second * sample_rate_));
}

core::nanoseconds_t SampleSpec::timestamp_to_ns(packet::timestamp_diff_t ts) const {
    return core::nanoseconds_t(roundf(float(ts) / sample_rate_ * core::Second));
}

size_t SampleSpec::ns_to_size(core::nanoseconds_t frame_length) const {
    return (size_t)timestamp_from_ns(frame_length) * num_channels_;
}

core::nanoseconds_t SampleSpec::size_to_ns(size_t frame_size) const {
    return timestamp_to_ns(packet::timestamp_diff_t(frame_size/num_channels_));
}

size_t SampleSpec::calc_num_channels() {
    size_t n_ch = 0;
    channel_mask_t ch_mask = channels_;
    for (; ch_mask != 0; ch_mask >>= 1) {
        if (ch_mask & 1) {
            n_ch++;
        }
    }
    return n_ch;
}

} // namespace audio
} // namespace ro
