/*
 * Copyright (c) 2017 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/isource.h"
#include "roc_core/panic.h"

namespace roc {
namespace sndio {

ISource::ISource(core::IArena& arena)
    : IDevice(arena) {
}

ISource::~ISource() {
}

} // namespace sndio
} // namespace roc
