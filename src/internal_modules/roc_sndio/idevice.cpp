/*
 * Copyright (c) 2022 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/idevice.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

IDevice::IDevice(core::IArena& arena)
    : core::ArenaAllocation(arena) {
}

IDevice::~IDevice() {
}

DeviceState IDevice::state() const {
    roc_panic_if_msg(has_state(),
                     "device: if has_state() is true, state() should be implemented");

    return DeviceState_Active;
}

status::StatusCode IDevice::pause() {
    roc_panic_if_msg(has_state(),
                     "device: if has_state() is true, pause() should be implemented");

    return status::StatusOK;
}

status::StatusCode IDevice::resume() {
    roc_panic_if_msg(has_state(),
                     "device: if has_state() is true, resume() should be implemented");

    return status::StatusOK;
}

core::nanoseconds_t IDevice::latency() const {
    roc_panic_if_msg(has_latency(),
                     "device: if has_latency() is true, latency() should be implemented");

    return 0;
}

} // namespace sndio
} // namespace roc
