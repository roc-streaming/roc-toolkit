/*
 * Copyright (c) 2015 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/iwriter.h
//! @brief Packet writer interface.

#ifndef ROC_PACKET_IWRITER_H_
#define ROC_PACKET_IWRITER_H_

#include "roc_packet/packet.h"

namespace roc {
namespace packet {

//! Packet writer interface.
class IWriter {
public:
    virtual ~IWriter();

    //! Write packet.
    virtual void write(const PacketPtr&) = 0;
};

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_IWRITER_H_
