/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/pulseaudio_sink.h"

namespace roc {
namespace sndio {

PulseaudioSink::PulseaudioSink(const Config& config)
    : PulseaudioDevice(config, DeviceType_Sink) {
}

PulseaudioSink::~PulseaudioSink() {
}

DeviceType PulseaudioSink::type() const {
    return DeviceType_Sink;
}

DeviceState PulseaudioSink::state() const {
    return PulseaudioDevice::state();
}

void PulseaudioSink::pause() {
    return PulseaudioDevice::pause();
}

bool PulseaudioSink::resume() {
    return PulseaudioDevice::resume();
}

bool PulseaudioSink::restart() {
    return PulseaudioDevice::restart();
}

audio::SampleSpec PulseaudioSink::sample_spec() const {
    return PulseaudioDevice::sample_spec();
}

core::nanoseconds_t PulseaudioSink::latency() const {
    return PulseaudioDevice::latency();
}

bool PulseaudioSink::has_latency() const {
    return true;
}

bool PulseaudioSink::has_clock() const {
    return true;
}

void PulseaudioSink::write(audio::Frame& frame) {
    PulseaudioDevice::request(frame);
}

} // namespace sndio
} // namespace roc
