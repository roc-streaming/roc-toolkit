/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/basic_control_endpoint.h"

namespace roc {
namespace ctl {

BasicControlEndpoint::BasicControlEndpoint(core::IArena& arena)
    : core::RefCounted<BasicControlEndpoint, core::ArenaAllocation>(arena) {
}

BasicControlEndpoint::~BasicControlEndpoint() {
}

} // namespace ctl
} // namespace roc
