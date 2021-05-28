/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_netio/basic_port.h"
#include "roc_core/log.h"

namespace roc {
namespace netio {

BasicPort::BasicPort(core::IAllocator& allocator)
    : allocator_(allocator) {
    descriptor_[0] = '\0';
}

BasicPort::~BasicPort() {
}

const char* BasicPort::descriptor() const {
    if (!descriptor_[0]) {
        roc_panic(
            "basic port: update_descriptor() was not called before calling descriptor()");
    }

    return descriptor_;
}

void BasicPort::update_descriptor() {
    core::StringBuilder b(descriptor_, sizeof(descriptor_));

    format_descriptor(b);

    if (!b.ok() || b.actual_size() == 0) {
        roc_panic("basic port: failed to format descriptor");
    }
}

core::IAllocator& BasicPort::allocator() {
    return allocator_;
}

void BasicPort::destroy() {
    allocator_.destroy(*this);
}

} // namespace netio
} // namespace roc
