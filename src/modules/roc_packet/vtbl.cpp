/*
 * Copyright (c) 2015 Mikhail Baranov
 * Copyright (c) 2015 Victor Gaydov
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "roc_packet/iheader_fecframe.h"
#include "roc_packet/iheader_ordering.h"
#include "roc_packet/iheader_rtp.h"
#include "roc_packet/imonitor.h"
#include "roc_packet/ipacket.h"
#include "roc_packet/ipacket_composer.h"
#include "roc_packet/ipacket_parser.h"
#include "roc_packet/ipacket_reader.h"
#include "roc_packet/ipacket_writer.h"
#include "roc_packet/ipayload_audio.h"

namespace roc {
namespace packet {

IPacketComposer::~IPacketComposer() {
}

IPacketParser::~IPacketParser() {
}

IPacketReader::~IPacketReader() {
}

IPacketWriter::~IPacketWriter() {
}

IPacketConstWriter::~IPacketConstWriter() {
}

IHeaderOrdering::~IHeaderOrdering() {
}

IHeaderRTP::~IHeaderRTP() {
}

IHeaderFECFrame::~IHeaderFECFrame() {
}

IPayloadAudio::~IPayloadAudio() {
}

IMonitor::~IMonitor() {
}

} // namespace packet
} // namespace roc
