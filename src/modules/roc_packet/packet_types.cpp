/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/iaudio_packet.h"
#include "roc_packet/ifec_packet.h"

#include "roc_packet/print_packet.h"

namespace roc {
namespace packet {

namespace {

char IAudioPacketType;
char IFECPacketType;

} // namespace

const PacketType IAudioPacket::Type = &IAudioPacketType;

IAudioPacket::~IAudioPacket() {
}

void IAudioPacket::print(bool body) const {
    print_packet(*this, body);
}
const PacketType IFECPacket::Type = &IFECPacketType;

IFECPacket::~IFECPacket() {
}

void IFECPacket::print(bool body) const {
    print_packet(*this, body);
}

} // namespace packet
} // namespace roc
