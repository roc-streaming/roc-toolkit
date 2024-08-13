/*
 * Copyright (c) 2019 Roc Streaming authors
 *
 * This Sink Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_sndio/isink.h"

namespace roc {
namespace sndio {

ISink::ISink(core::IArena& arena)
    : IDevice(arena) {
}

ISink::~ISink() {
}

} // namespace sndio
} // namespace roc
