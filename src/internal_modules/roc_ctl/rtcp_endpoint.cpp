/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_ctl/rtcp_endpoint.h"

namespace roc {
namespace ctl {

RtcpEndpoint::RtcpEndpoint(core::IAllocator& allocator)
    : BasicControlEndpoint(allocator) {
}

bool RtcpEndpoint::bind(const address::EndpointUri& uri) {
    // TODO
    (void)uri;
    return false;
}

bool RtcpEndpoint::connect(const address::EndpointUri& uri) {
    // TODO
    (void)uri;
    return false;
}

void RtcpEndpoint::close() {
    // TODO
}

} // namespace ctl
} // namespace roc
