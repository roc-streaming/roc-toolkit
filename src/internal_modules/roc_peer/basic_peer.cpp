/*
 * Copyright (c) 2020 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_peer/basic_peer.h"

namespace roc {
namespace peer {

BasicPeer::BasicPeer(Context& context)
    : context_(context) {
    context_.incref();
}

BasicPeer::~BasicPeer() {
    context_.decref();
}

void BasicPeer::destroy() {
    context_.allocator().destroy(*this);
}

} // namespace peer
} // namespace roc
