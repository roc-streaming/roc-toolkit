/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/pulseaudio_source.h"
#include "roc_sndio/device_type.h"
#include "roc_sndio/pulseaudio_device.h"

namespace roc {
namespace sndio {

PulseaudioSource::PulseaudioSource(const Config& config)
    : PulseaudioDevice(config, DeviceType_Source) {
}

PulseaudioSource::~PulseaudioSource() {
}

DeviceType PulseaudioSource::type() const {
    return DeviceType_Source;
}

DeviceState PulseaudioSource::state() const {
    return PulseaudioDevice::state();
}

void PulseaudioSource::pause() {
    return PulseaudioDevice::pause();
}

bool PulseaudioSource::resume() {
    return PulseaudioDevice::resume();
}

bool PulseaudioSource::restart() {
    return PulseaudioDevice::restart();
}

audio::SampleSpec PulseaudioSource::sample_spec() const {
    return PulseaudioDevice::sample_spec();
}

core::nanoseconds_t PulseaudioSource::latency() const {
    return PulseaudioDevice::latency();
}

bool PulseaudioSource::has_latency() const {
    return true;
}

bool PulseaudioSource::has_clock() const {
    return true;
}

void PulseaudioSource::reclock(core::nanoseconds_t) {
    // no-op
}

bool PulseaudioSource::read(audio::Frame& frame) {
    return PulseaudioDevice::request(frame);
}

} // namespace sndio
} // namespace roc
