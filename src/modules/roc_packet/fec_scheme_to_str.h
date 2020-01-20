/*
 * Copyright (c) 2019 Roc authors
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

//! @file roc_packet/fec_scheme_to_str.h
//! @brief FEC scheme to string.

#ifndef ROC_PACKET_FEC_SCHEME_TO_STR_H_
#define ROC_PACKET_FEC_SCHEME_TO_STR_H_

#include "roc_packet/fec.h"

namespace roc {
namespace packet {

//! FEC scheme to string.
const char* fec_scheme_to_str(FecScheme);

} // namespace packet
} // namespace roc

#endif // ROC_PACKET_FEC_SCHEME_TO_STR_H_
