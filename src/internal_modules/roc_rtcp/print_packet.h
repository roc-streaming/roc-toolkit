/*
 * Copyright (c) 2022 Roc Streaming authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_rtcp/print_packet.h
//! @brief Print RTCP packet to console.

#ifndef ROC_RTCP_PRINT_PACKET_H_
#define ROC_RTCP_PRINT_PACKET_H_

#include "roc_core/slice.h"
#include "roc_core/stddefs.h"

namespace roc {
namespace rtcp {

//! Print RTCP compund packet.
void print_packet(const core::Slice<uint8_t>& data);

} // namespace rtcp
} // namespace roc

#endif // ROC_RTCP_PRINT_PACKET_H_
