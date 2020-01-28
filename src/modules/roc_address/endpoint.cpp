/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_address/endpoint.h"
#include "roc_core/log.h"
#include "roc_core/panic.h"

namespace roc {
namespace address {

Endpoint::Endpoint(core::IAllocator& allocator)
    : allocator_(allocator)
    , invalid_parts_(0)
    , uri_(allocator)
    , miface_(allocator)
    , broadcast_(false) {
}

void Endpoint::destroy() {
    allocator_.destroy(*this);
}

bool Endpoint::check() const {
    if (invalid_parts_ != 0) {
        roc_log(LogError, "invalid endpoint: contains invalid parts");
        return false;
    }

    if (!uri_.check(EndpointURI::Subset_Full)) {
        return false;
    }

    return true;
}

bool Endpoint::part_is_valid_(Part part) const {
    return (invalid_parts_ & part) == 0;
}

void Endpoint::set_valid_(Part part) {
    invalid_parts_ &= ~part;
}

void Endpoint::set_invalid_(Part part) {
    invalid_parts_ |= part;
}

const EndpointURI& Endpoint::uri() const {
    return uri_;
}

EndpointURI& Endpoint::uri() {
    return uri_;
}

const char* Endpoint::miface() const {
    if (!part_is_valid_(PartMiface) || miface_.is_empty()) {
        return NULL;
    }
    return miface_.c_str();
}

bool Endpoint::set_miface(const char* str) {
    if (!str) {
        miface_.clear();
        set_valid_(PartMiface);
        return true;
    }

    if (!miface_.set_str(str)) {
        set_invalid_(PartMiface);
        return false;
    }

    set_valid_(PartMiface);
    return true;
}

bool Endpoint::format_miface(core::StringBuilder& dst) const {
    if (!part_is_valid_(PartMiface) || miface_.is_empty()) {
        return false;
    }
    dst.append_str(miface_.c_str());
    return true;
}

bool Endpoint::broadcast() const {
    if (!part_is_valid_(PartBroadcast)) {
        return false;
    }
    return broadcast_;
}

bool Endpoint::set_broadcast(int flag) {
    if (flag != 0 && flag != 1) {
        set_invalid_(PartBroadcast);
        return false;
    }

    broadcast_ = flag;

    set_valid_(PartBroadcast);
    return true;
}

bool Endpoint::get_broadcast(int& flag) const {
    if (!part_is_valid_(PartBroadcast)) {
        return false;
    }

    flag = broadcast_;
    return true;
}

} // namespace address
} // namespace roc
