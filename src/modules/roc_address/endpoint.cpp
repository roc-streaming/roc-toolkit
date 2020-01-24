/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint.h"
#include "roc_core/panic.h"
#include "roc_core/string_utils.h"

namespace roc {
namespace address {

Endpoint::Endpoint(core::IAllocator& allocator)
    : uri_(allocator)
    , miface_(allocator)
    , broadcast_(false) {
}

const EndpointURI& Endpoint::uri() const {
    return uri_;
}

EndpointURI& Endpoint::uri() {
    return uri_;
}

const char* Endpoint::miface() const {
    return miface_.data();
}

bool Endpoint::set_miface(const char* str) {
    roc_panic_if(!str);

    const size_t str_len = strlen(str);

    if (str_len < 1) {
        miface_.resize(0);
        return true;
    }

    if (!miface_.resize(str_len + 1)) {
        return false;
    }

    if (!core::copy_str(miface_.data(), miface_.size(), str, str + str_len)) {
        return false;
    }

    return true;
}

bool Endpoint::broadcast() const {
    return broadcast_;
}

void Endpoint::set_broadcast(bool br) {
    broadcast_ = br;
}

} // namespace address
} // namespace roc
