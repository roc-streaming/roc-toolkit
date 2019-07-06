/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/basic_port.h"

namespace roc {
namespace netio {

BasicPort::BasicPort(core::IAllocator& allocator)
    : allocator_(allocator) {
}

BasicPort::~BasicPort() {
}

void BasicPort::destroy() {
    allocator_.destroy(*this);
}

} // namespace netio
} // namespace roc
