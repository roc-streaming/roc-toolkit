/*
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/ipacket.h"
#include "roc_packet/print_packet.h"

namespace roc {
namespace packet {

IPacket::~IPacket() {
}

void IPacket::print(bool print_payload) const {
    print_packet(*this, print_payload);
}

} // namespace packet
} // namespace roc
